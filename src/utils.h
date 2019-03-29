#ifndef UTILS
#define UTILS

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctime>
#include <chrono>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <type_traits>

enum RESULTS {
  OK = 0,
  AC = 10,
  WA,
  TLE,
  MLE,
  OLE,
  RE,
  SIG,
  CE,
  CO,
  ER
};

extern bool enable_log;

template <class T> void LogNL(const T& str) {
  if (enable_log) std::cerr << str;
}
template <class T> void LogNL(const std::vector<T>& vec) {
  if (enable_log && vec.size()) {
    std::cerr << vec[0];
    for (size_t i = 1; i < vec.size(); i++) std::cerr << ' ' << vec[i];
  }
}

template <class T, class... U> void LogNL(const T& str, U&&... tail) {
  if (enable_log) {
    LogNL(str);
    std::cerr << ' ';
    LogNL(std::forward<U>(tail)...);
  }
}

template <class... T> void Log(T&&... param) {
  static char buf[100];
  if (enable_log) {
    using namespace std::chrono;
    auto tnow = system_clock::now();
    time_t now = system_clock::to_time_t(tnow);
    int milli = duration_cast<milliseconds>(tnow - time_point_cast<seconds>(tnow)).count();
    strftime(buf, 100, "%F %T ", localtime(&now));
    std::cerr << buf << std::setfill('0') << std::setw(3) << milli << " -- ";
    LogNL(std::forward<T>(param)...);
    std::cerr << std::endl;
  }
}

inline const char* CStr(const char* x) { return x; }
inline const char* CStr(const std::string& x) { return x.c_str(); }

inline void RedirFd(const std::string& in, const std::string& out,
                    const std::string& err) {
  if (in.size()) {
    int fd = open(in.c_str(), O_RDONLY);
    if (fd < 0 || dup2(fd, 0) < 0) exit(1);
    close(fd);
  }
  if (out.size()) {
    int fd = open(out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0 || dup2(fd, 1) < 0) exit(1);
    close(fd);
  }
  if (err.size()) {
    int fd = open(out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0 || dup2(fd, 2) < 0) exit(1);
    close(fd);
  }
}
template <class Func> int ExecHelper(Func f) {
  int pid = fork();
  if (pid < 0) return -1;
  if (pid == 0) {
    f();
    exit(1);
  } else {
    int status;
    waitpid(pid, &status, 0);
    return status;
  }
}

inline std::string RedirStr(const std::string& in, const std::string& out,
    const std::string& err) {
  std::string redir;
  if (in.size()) redir += "<" + in;
  if (out.size()) redir += " >" + out;
  if (err.size()) redir += " 2>" + err;
  return redir;
}

inline void ExecuteList(const std::vector<std::string>& args) {
  std::vector<char*> argvec(args.size() + 1, nullptr);
  for (size_t i = 0; i < args.size(); i++) {
    argvec[i] = const_cast<char*>(args[i].c_str());
  }
  execvp(args[0].c_str(), argvec.data());
}

template <class U, class... T> int ExecuteRedir(const std::string& in,
    const std::string& out, const std::string& err, U&& first, T&&... args) {
  if (enable_log) {
    std::string redir;
    if (in.size()) redir += " <" + in;
    if (out.size()) redir += " >" + out;
    if (err.size()) redir += " 2>" + err;
    Log("Execute command:", first, args..., redir);
  }
  return ExecHelper([&](){
    RedirFd(in, out, err);
    execlp(CStr(first), CStr(first), CStr(args)..., nullptr);
  });
}
template <class U, class... T> int Execute(U&& first, T&&... args) {
  Log("Execute command:", first, args...);
  return ExecHelper([&](){
    execlp(CStr(first), CStr(first), CStr(args)..., nullptr);
  });
}

template <class T> struct IsStringVec :
    std::enable_if<std::is_same<typename std::decay<T>::type,
                                std::vector<std::string>>::value>
{};
template <class T, class = IsStringVec<T>>
int ExecuteRedir(const std::string& in, const std::string& out,
                 const std::string& err, T&& args) {
  if (enable_log) Log("Execute command:", args, RedirStr(in, out, err));
  return ExecHelper([&](){ RedirFd(in, out, err); ExecuteList(args); });
}
template <class T, class = IsStringVec<T>> int Execute(T&& args) {
  Log("Execute command: ", args);
  return ExecHelper([&](){ ExecuteList(args); });
}

extern const std::string kBoxRoot;
extern const std::string kTestdataRoot;

std::string PadInt(int x, size_t width = 0);
std::string BoxPath(int box);
std::string BoxInput(int box);
std::string BoxOutput(int box);
std::string TdPath(int prob);
std::string TdMeta(int prob, int td);
std::string TdInput(int prob, int td);
std::string TdOutput(int prob, int td);

class fromVerdict{
  const int verdict;
 public:
  explicit fromVerdict();
  explicit fromVerdict(int verdict) : verdict(verdict) {}
  const char* toStr() const {
    switch(verdict){
      case AC:
        return "Accepted";
      case WA:
        return "Wrong Answer";
      case TLE:
        return "Time Limit Exceeded";
      case MLE:
        return "Segmentation Fault";
      case OLE:
        return "Output Limit Exceeded";
      case RE:
        return "Runtime Error (exited with nonzero status)";
      case SIG:
        return "Runtime Error (exited with signal)";
      case CE:
        return "Compile Error";
      case CO:
        return "Compilation Timed Out";
      case ER:
        return "WTF!";
                              default:
                                      return "nil";
    }
  }
  std::string toAbr() const {
    switch(verdict){
      case AC:
        return "AC";
      case WA:
        return "WA";
      case TLE:
        return "TLE";
      case MLE:
        return "MLE";
      case OLE:
        return "OLE";
      case RE:
        return "RE";
      case SIG:
        return "SIG";
      case CE:
        return "CE";
      case CO:
        return "CO";
      case ER:
        return "ER";
      default:
        return "";
    }
  }
};

class cast {
  const std::string val;
 public:
  explicit cast();
  cast(const std::string& c) : val(c) {}
  template <typename T> T to() const {
    T res;
    std::istringstream in(val);
    in >> res;
    return res;
  }
};

class submission {
 public:
  //meta
  int problem_id;
  int submission_id;
  std::string code;
  std::string sjcode;
  std::string interlib;
  std::string lang;
  std::string std;
  std::string submitter;
  int submitter_id;
  //problem
  int problem_type;
  std::string special_judge;
  //test result
  struct Testdata {
    int mem_limit, time_limit, output_limit, verdict, mem, time;
  };
  std::vector<Testdata> tds;

  submission() : problem_id(0), submission_id(0), submitter_id(0),
      problem_type(0) {}
};

#endif
