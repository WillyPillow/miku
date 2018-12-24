#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/select.h>

#include <fstream>
#include <sstream>

#include "server_io.h"

struct Reply {
  uint16_t query_id;
  uint8_t status;
  std::string message;
};

enum QueryType : uint8_t {
  kFetchCode = 0,
  kFetchInterlib = 1,
  kFetchLimits = 2,
  kFetchSjCode = 3,
  kFetchSubmission = 4,
  kFetchTestdata = 5,
  kFetchTestdataMeta = 6,
  kRespondValidating = 7,
  kUpdateMessage = 8,
  kUpdateVerdict = 9,
  kInitialize = 255
};

static pid_t py_pid;
static int infd, outfd;
static uint16_t query_id;

inline int ReadReply(Reply& rep) {
  uint8_t header[8];
  if (read(infd, (char*)header, 8) != 8) return -1;
  rep.query_id = header[0] | (uint16_t)header[1] << 8;
  rep.status = header[2];
  uint32_t sz = header[7];
  sz = sz << 8 | header[6];
  sz = sz << 8 | header[5];
  sz = sz << 8 | header[4];
  rep.message.resize(sz);
  if (sz && read(infd, rep.message.data(), sz) != sz) return -1;
  return 0;
}

inline uint16_t SendMsg(QueryType qu, const std::string& str, bool noreply = false) {
  uint8_t header[8];
  header[0] = static_cast<uint8_t>(qu);
  header[1] = noreply;
  header[2] = query_id & 255;
  header[3] = query_id >> 8;
  header[4] = str.size() & 255;
  header[5] = str.size() >> 8 & 255;
  header[6] = str.size() >> 16 & 255;
  header[7] = str.size() >> 24 & 255;
  write(outfd, header, 8);
  write(outfd, str.data(), str.size());
  return query_id++;
}

bool InitServerIO() {
  int pipefd[2][2];
  for (int i : {0, 1}) {
    if (pipe(pipefd[i]) < 0) return false;
  }
  infd = pipefd[0][0];
  outfd = pipefd[1][1];
  query_id = 0;
  py_pid = fork();
  if (py_pid < 0) return false;
  if (!py_pid) {
    dup2(pipefd[0][1], 1);
    dup2(pipefd[1][0], 0);
    for (int i : {0, 1}) for (int j : {0, 1}) close(pipefd[i][j]);
    execlp("server_io.py", "server_io.py", nullptr);
    exit(1);
  }
  close(pipefd[0][1]);
  close(pipefd[1][0]);
  SendMsg(kInitialize, "");
  Reply rep;
  if (ReadReply(rep) != 0) return false;
  return rep.query_id == 0 && rep.status == 0 && rep.message.empty();
}

inline Reply SendCmd(QueryType qu, const std::string& str, bool noreply = false) {
  uint16_t id = SendMsg(qu, str, noreply);
  Reply rep;
  if (ReadReply(rep) != 0 || id != rep.query_id) {
    Log("Error: Unexpected reply");
    exit(1);
  }
  return rep;
}

void respondValidating(int sid) {
  SendMsg(kRespondValidating, PadInt(sid), true);
}

int fetchSubmission(submission& sub) {
  Reply rep = SendCmd(kFetchSubmission, "");
  std::stringstream ss(rep.message);
  ss >> sub.submission_id;
  if (sub.submission_id < 0) return sub.submission_id;

  std::string lang;
  ss >> sub.problem_id >> sub.problem_type >> sub.submitter_id >> lang;
  if (lang == "c++11") {
    sub.lang = "c++";
    sub.std = "c++11";
  } else if (lang == "c++14") {
    sub.lang = "c++";
    sub.std = "c++14";
  } else if (lang == "c++17") {
    sub.lang = "c++";
    sub.std = "c++17";
  } else if (lang == "c++98") {
    sub.lang = "c++";
    sub.std = "c++98";
  } else if (lang == "c90") {
    sub.lang = "c";
    sub.std = "c90";
  } else if (lang == "c99") {
    sub.lang = "c";
    sub.std = "c99";
  } else if (lang == "c11") {
    sub.lang = "c";
    sub.std = "c11";
  } else if (lang == "haskell") {
    sub.lang = "haskell";
    sub.std = "";
  } else if (lang == "python2") {
    sub.lang = "python2";
    sub.std = "";
  } else if (lang == "python3") {
    sub.lang = "python3";
    sub.std = "";
  } else if (lang == "c++") {
    sub.lang = "c++";
    sub.std = "c++14";
  } else if (lang == "c") {
    sub.lang = "c";
    sub.std = "c11";
  } else {
    sub.lang = "c++";
    sub.std = "c++11";
  }
  return 0;
}

int downloadTestdata(submission &sub) {
  Reply rep = SendCmd(kFetchTestdataMeta, PadInt(sub.problem_id));
  if (rep.status) return -1;
  std::stringstream ss(rep.message);
  int tdcount = 0;
  ss >> tdcount;
  sub.tds.resize(tdcount);
  for (int i = 0; i < tdcount; ++i) {
    int testdata_id;
    long long timestamp;
    ss >> testdata_id >> timestamp;
    long long ts;
    std::string metafile = TdMeta(sub.problem_id, i);
    std::ifstream fin(metafile);
    bool flag = !(fin >> ts && ts == timestamp);
    fin.close();
    if (flag) {
      // renew td
      Log("Renew testdata:", testdata_id, sub.problem_id, i);
      if (SendCmd(kFetchTestdata, PadInt(testdata_id) + '\0' +
          PadInt(sub.problem_id) + '\0' + PadInt(i)).status) return -1;
      std::ofstream fout(metafile);
      fout << timestamp << '\n';
    }
  }
  return 0;
}

int fetchProblem(submission &sub) {
  std::string pid = PadInt(sub.problem_id);
  { // Get submission code
    Reply rep = SendCmd(kFetchCode, PadInt(sub.submission_id));
    if (rep.status) return -1;
    rep.message.swap(sub.code);
  } { // Get special judge code
    Reply rep = SendCmd(kFetchSjCode, pid);
    if (rep.status) return -1;
    rep.message.swap(sub.sjcode);
  } { // Get Interactive lib
    Reply rep = SendCmd(kFetchInterlib, pid);
    if (rep.status) return -1;
    rep.message.swap(sub.interlib);
  }
  // Download testdata (create dir if not exist)
  if (downloadTestdata(sub) == -1) return -1;

  // Get limits
  Reply rep = SendCmd(kFetchLimits, pid);
  if (rep.status) return -1;
  std::stringstream ss(rep.message);
  for (auto& i : sub.tds) ss >> i.time_limit >> i.mem_limit;
  return 0;
}

int sendResult(submission &sub, int verdict, bool done) {
  std::string result;
  if (verdict == CE) {
    result = "CE";
  } else if(verdict == ER) {
    result = "ER";
  } else {
    for (size_t i = 0; i < sub.tds.size(); ++i) {
      auto& nowtd = sub.tds[i];
      result += fromVerdict(nowtd.verdict).toAbr() + '/' +
          PadInt(nowtd.time) + '/' + PadInt(nowtd.mem) + '/';
      Log("td", i, ": time", nowtd.time, "mem", nowtd.mem, "verdict",
          fromVerdict(nowtd.verdict).toStr());
    }
  }
  SendMsg(kUpdateVerdict, PadInt(sub.submission_id) + '\0' +
      result + '\0' + (done ? "OK" : "NO"), true);
  return 0;
}

int sendMessage(const submission &sub, const std::string& message) {
  SendMsg(kUpdateMessage, PadInt(sub.submission_id) + '\0' + message, true);
  return 0;
}
