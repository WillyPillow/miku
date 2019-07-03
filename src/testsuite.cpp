#include <iostream>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <map>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"
#include "sandbox.h"
#include "config.h"
#include "testsuite.h"
#include "server_io.h"

using namespace std;

int compile(const submission& target, int boxid, int spBoxid);
void eval(submission &sub, int td, int boxid, int spBoxid);
void getExitStatus(submission &sub, int td);

int MAXPARNUM = 1;
int BOXOFFSET = 10;
bool AGGUPDATE = false;

int testsuite(submission &sub) {
   Execute("rm", "-f", "./testzone/*");
   const int testBoxid = BOXOFFSET + 0, spBoxid = BOXOFFSET + 1;
   auto DelSandboxes = [&](){
      sandboxDele(testBoxid);
      sandboxDele(spBoxid);
   };

   sandboxInit(testBoxid);
   sandboxInit(spBoxid);
   int status = compile(sub, testBoxid, spBoxid);
   if(status != OK) {
      DelSandboxes();
      return status;
   }

   //anyway, only have batch judge right now
   map<pid_t, int> proc;
   int procnum = 0;
   int problem_id = sub.problem_id;

   for (size_t i = 0; i < sub.tds.size(); ++i) {
      auto& nowtd = sub.tds[i];
      while(procnum >= MAXPARNUM){
         int status;
         pid_t cid = waitpid(-1, &status, 0);
         if(cid == -1){
            perror("[ERROR] in testsuite,  `waitpid()` failed :");
            DelSandboxes();
            return ER;
         }
         int td = proc[cid];
         eval(sub, td, BOXOFFSET + 10 + td, spBoxid);
         if(AGGUPDATE){
            sendResult(sub, OK, false);
         }
         //cerr << "td" << td << " : " << sub.verdict[td] << endl;
         sandboxDele(BOXOFFSET + 10 + td);
         --procnum;
      }

      if(procnum < MAXPARNUM){
         //batch judge
         int memlim = nowtd.mem_limit;
         if (sub.lang == "haskell") memlim = memlim * 5 + 24 * 1024;
         pid_t pid = fork();
         if(pid == -1){
            perror("[ERROR] in testsuite, `fork()` failed :");
            DelSandboxes();
            return ER;
         }
         if(pid == 0){
            //child proc
            std::vector<std::string> cmd = {
                "batchjudge", PadInt(problem_id), PadInt(i),
                PadInt(BOXOFFSET + 10 + i), PadInt(nowtd.time_limit),
                PadInt(memlim), PadInt(nowtd.output_limit),
                PadInt(testBoxid), sub.lang, PadInt(enable_log)};
#define LS(x) cmd[x].c_str()
            execlp(LS(0),LS(0),LS(1),LS(2),LS(3),LS(4),LS(5),LS(6),LS(7),LS(8),LS(9),nullptr);
#undef LS
            perror("[ERROR] in testsuite, `execl()` failed :");
            exit(0);
         }
         proc[pid] = i;
         ++procnum;
      }
   }
   while(procnum >= 1){
      int status;
      pid_t cid = waitpid(-1, &status, 0);
      if(cid == -1){
         perror("[ERROR] in testsuite,  `waitpid()` failed :");
         DelSandboxes();
         return ER;
      }
      const int td = proc[cid];
      //sub.verdict[td] = eval(problem_id, td);
      eval(sub, td, BOXOFFSET + 10 + td, spBoxid);
      if(AGGUPDATE){
         sendResult(sub, OK, false);
      }
      //cerr << "td" << td << " : " << sub.verdict[td] << endl;
      sandboxDele(BOXOFFSET + 10 + td);
      --procnum;
   }
   //clear box-10
   DelSandboxes();
   return OK;
}

void setExitStatus(submission &sub, int td)
{
   ifstream fin("./testzone/META" + PadInt(td));
   string line;
   map<string,string> META;
   while(!fin.eof() && getline(fin, line)){
      for(size_t i = 0; i < line.size(); ++i)
         if(line[i] == ':')
            line[i] = ' ';
      istringstream in(line);
      string a, b;
      in >> a >> b;
      META[a] = b;
   }

   auto& nowtd = sub.tds[td];
   // mem_used
   nowtd.mem = cast(META["max-rss"]).to<int>();
   // time_used
   nowtd.time = 1000 * cast(META["time"]).to<double>();
   // verdict
   if(META["status"] == ""){
      nowtd.verdict = OK;
   }else if(META["status"] == "TO"){
      nowtd.verdict = TLE;
   }else if(META["status"] == "SG"){
      switch(cast(META["exitsig"]).to<int>()){
         case 11:
            nowtd.verdict = MLE;
            break;
         default :
            nowtd.verdict = RE;
      }
   }else if(META["status"] == "RE"){
      nowtd.verdict = RE;
   }else{
      nowtd.verdict = ER;
   }
}

void eval(submission &sub, int td, int boxid, int spBoxid)
{
   auto& nowtd = sub.tds[td];
   int problem_id = sub.problem_id;
   setExitStatus(sub, td);
   if(nowtd.verdict != OK){
      return ;
   }

   if(sub.problem_type == 1){
      string cmd;
      {
        cmd += BoxPath(spBoxid) + "sj.out ";
        cmd += BoxPath(boxid) + "output ";
        cmd += TdInput(problem_id, td) + ' ';
        cmd += TdOutput(problem_id, td) + ' ';
        cmd += (sub.std.empty() ? sub.lang : sub.std) + ' ';

        string codefile = BoxPath(boxid) + "code";
        cmd += codefile;
        ofstream fout(codefile);
        fout.write(sub.code.c_str(), sub.code.size());
      }

      FILE* Pipe = popen(cmd.c_str(), "r");
      int result = 1;
      fscanf(Pipe, "%d", &result);
      pclose(Pipe);
      Log("[special judge] :", cmd);
      Log("[special judge] td:", td, " result:", result);
      nowtd.verdict = result ? WA : AC;
      return;
   }

   int status = AC;
   //solution output
   fstream tsol(TdOutput(problem_id, td));
   //user output
   std::string output_file = BoxPath(boxid) + "output";
   { // check if output is regular file
      struct stat output_stat;
      if (stat(output_file.c_str(), &output_stat) < 0 || !S_ISREG(output_stat.st_mode)) {
         nowtd.verdict = WA;
         return;
      }
   }

   fstream mout(output_file);
   while(true){
      if(tsol.eof() != mout.eof()){
         while(tsol.eof() != mout.eof()){
            string s;
            if(tsol.eof()){
               getline(mout, s);
            }else{
               getline(tsol, s);
            }
            s.erase(s.find_last_not_of(" \n\r\t") + 1);
            if(s != ""){
               status = WA;
               break;
            }
         }
         break;
      }
      if(tsol.eof() && mout.eof()){
         break;
      }
      string s, t;
      getline(tsol, s);
      getline(mout, t);
      s.erase(s.find_last_not_of(" \n\r\t") + 1);
      t.erase(t.find_last_not_of(" \n\r\t") + 1);
      if(s != t){
         status = WA;
         break;
      }
   }
   nowtd.verdict = status;
}

int compile(const submission& target, int boxid, int spBoxid)
{
   string boxdir = BoxPath(boxid);

   ofstream fout;
   if(target.lang == "c++"){
      fout.open(boxdir + "main.cpp");
   }else if(target.lang == "c"){
      fout.open(boxdir + "main.c");
   }else if(target.lang == "haskell"){
      fout.open(boxdir + "main.hs");
   }else if(target.lang == "python2" || target.lang == "python3"){
      fout.open(boxdir + "main.py");
   }else{
      return CE;
   }
   fout.write(target.code.c_str(), target.code.size());
   fout.close();

   if(target.problem_type == 2){
      fout.open(boxdir + "lib" + PadInt(target.problem_id, 4)  + ".h");
      fout << target.interlib;
      fout.close();
   }

  std::vector<std::string> args;
  if(target.lang == "c++"){
    args = {"/usr/bin/env", "g++", "./main.cpp", "-o", "./main.out", "-O2", "-w"};
  }else if(target.lang == "c"){
    if(target.std == "" || target.std == "c90"){
      args = {"/usr/bin/env", "gcc", "./main.c", "-o", "./main.out", "-O2", "-ansi", "-lm", "-w"};
    }else{
      args = {"/usr/bin/env", "gcc", "./main.c", "-o", "./main.out", "-O2", "-lm", "-w"};
    }
  }else if(target.lang == "haskell"){
    args = {"/usr/bin/env", "ghc", "./main.hs", "-o", "./main.out", "-O", "-tmpdir", ".", "-w"};
  }else if(target.lang == "python2"){
    args = {"/usr/bin/env", "python2.7", "-m", "py_compile", "main.py"};
  }else if(target.lang == "python3"){
    args = {"/usr/bin/env", "python3.7", "-c", "import py_compile;py_compile.compile(\'main.py\',\'main.pyc\')"};
  }
  if(!target.std.empty() && target.std != "c90"){
    args.push_back("-std=" + target.std);
  }

   sandboxOptions opt;
   opt.dirs.push_back("/etc/alternatives");
   if (target.lang == "haskell") opt.dirs.push_back("/var");
   opt.procs = 50;
   opt.preserve_env = true;
   opt.errout = "compile_error";
   opt.output = "/dev/null";
   opt.timeout = 30 * 1000;
   opt.meta = "./testzone/metacomp";
   opt.fsize_limit = 32768;

   sandboxExec(boxid, opt, args);
   string compiled_target;
   if(target.lang == "python2" || target.lang == "python3")
     compiled_target = "main.pyc";
   else
     compiled_target = "main.out";
   if(access((boxdir + compiled_target).c_str(), F_OK) == -1){
      if(access((boxdir+"compile_error").c_str(), F_OK) == 0){
          const int MAX_MSG_LENGTH = 3000;
          ifstream compile_error_msg((boxdir+"compile_error").c_str());
          char cerr_msg_cstring[MAX_MSG_LENGTH + 5] = "";
          compile_error_msg.read(cerr_msg_cstring, MAX_MSG_LENGTH + 1);
          string cerr_msg(cerr_msg_cstring);
          if(cerr_msg.size() > MAX_MSG_LENGTH){
             cerr_msg = regex_replace(cerr_msg.substr(0, MAX_MSG_LENGTH),
                   regex("(^|\\n)In file included from[\\S\\s]*?((\\n\\./main\\.c)|($))"),
                   "$1[Error messages from headers removed]$2");
             cerr_msg += "\n(Error message truncated after " + to_string(MAX_MSG_LENGTH) + " Bytes.)";
          } else {
             cerr_msg = regex_replace(cerr_msg,
                   regex("(^|\\n)In file included from[\\S\\s]*?((\\n\\./main\\.c)|($))"),
                   "$1[Error messages from headers removed]$2");
          }
          sendMessage(target, cerr_msg);
      }
      return CE;
   }

   if(target.problem_type == 1){
      string spBoxdir(BoxPath(spBoxid));

      fout.open(spBoxdir + "sj.cpp");
      fout << target.sjcode;
      fout.close();

      std::vector<std::string> args{"/usr/bin/env", "g++", "./sj.cpp", "-o",
          "./sj.out", "-O2", "-std=c++14"};
      opt.meta = "./testzone/metacompsj";

      sandboxExec(spBoxid, opt, args);
      if(access((spBoxdir+"sj.out").c_str(), F_OK) == -1){
         return ER;
      }
   }

   return OK;
}
