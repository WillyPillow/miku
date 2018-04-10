#include<iostream>
#include<cstdio>
#include<sstream>
#include<fstream>
#include<iomanip>
#include<regex>
#include<map>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include"utils.h"
#include"sandbox.h"
#include"config.h"
#include"testsuite.h"
#include"server_io.h"

using namespace std;

int compile(const submission& target, int boxid, int spBoxid);
void eval(submission &sub, int td, int boxid, int spBoxid);
void getExitStatus(submission &sub, int td);

int MAXPARNUM = 1;
int BOXOFFSET = 10;
bool AGGUPDATE = false;

int testsuite(submission &sub)
{
   system("rm -f ./testzone/*");
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
   int* time_limit = sub.time_limit;
   int* mem_limit = sub.mem_limit;

   for(int i = 0; i < sub.testdata_count; ++i){
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
         ostringstream command;
         command << "batchjudge " << problem_id;
         command << " " << i;
         command << " " << BOXOFFSET + 10 + i;
         command << " " << time_limit[i];
         if(sub.lang == "haskell")
           command << " " << mem_limit[i]*5 + 24*1024;
         else
           command << " " << mem_limit[i];
         command << " " << testBoxid;
         if(sub.lang == "python2") 
           command << " " << "main.pyc";
         else
           command << " " << "main.out";
         pid_t pid = fork();
         if(pid == -1){
            perror("[ERROR] in testsuite, `fork()` failed :");
            DelSandboxes();
            return ER;
         }
         if(pid == 0){
            //child proc
            execl("/bin/sh", "sh", "-c", command.str().c_str(), NULL);
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
   ostringstream sout;
   sout << "./testzone/META" << td;
   ifstream fin(sout.str());
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

   //mem_used
   sub.mem[td] = cast(META["max-rss"]).to<int>();
   //time_used
   sub.time[td] = 1000 * cast(META["time"]).to<double>();
   //verdict
   if(META["status"] == ""){
      sub.verdict[td] = OK;
   }else if(META["status"] == "TO"){
      sub.verdict[td] = TLE;
   }else if(META["status"] == "SG"){
      switch(cast(META["exitsig"]).to<int>()){
         case 11:
            sub.verdict[td] = MLE;
            break;
         default :
            sub.verdict[td] = RE;
      }
   }else if(META["status"] == "RE"){
      sub.verdict[td] = RE;
   }else{
      // "XX"
      sub.verdict[td] = ER;
   }
   return ;
}

void eval(submission &sub, int td, int boxid, int spBoxid)
{
   int problem_id = sub.problem_id;
   setExitStatus(sub, td);
   if(sub.verdict[td] != OK){
      return ;
   }

   if(sub.problem_type == 1){
      ostringstream sout;
      sout << "/tmp/box/" << spBoxid << "/box/sj.out ";
      string sjpath(sout.str());
      sout.str("");
      sout << "./testdata/" << setfill('0') << setw(4) << problem_id << "/input" << setw(3) << td << ' ';
      string tdin(sout.str());
      sout.str("");
      sout << "./testdata/" << setfill('0') << setw(4) << problem_id << "/output" << setw(3) << td << ' ';
      string tdout(sout.str());
      sout.str("");
      sout << "/tmp/box/" << boxid << "/box/output ";
      string ttout(sout.str());
      FILE* Pipe = popen((sjpath+ttout+tdin+tdout).c_str(), "r");
      int result = 1;
      fscanf(Pipe, "%d", &result);
      pclose(Pipe);
      Log("[special judge] :", sjpath+ttout+tdin+tdout);
      Log("[special judge] td:", td, " result:", result);
      if(result == 0)
         sub.verdict[td] = AC;
      else
         sub.verdict[td] = WA;
      return ;
   }

   int status = AC;
   //solution output
   ostringstream sout;
   sout << "./testdata/" << setfill('0') << setw(4) << problem_id
      << "/output" << setw(3) << td;
   fstream tsol(sout.str());
   //user output
   sout.str("");
   sout << "/tmp/box/" << boxid << "/box/output";
   { // check if output is regular file
      struct stat output_stat;
      if (stat(sout.str().c_str(), &output_stat) < 0 || !S_ISREG(output_stat.st_mode)) {
         sub.verdict[td] = WA;
         return;
      }
   }

   fstream mout(sout.str());
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
   sub.verdict[td] = status;
   return ;
}

int compile(const submission& target, int boxid, int spBoxid)
{
   ostringstream sout;
   sout << "/tmp/box/" << boxid << "/box/";
   string boxdir(sout.str());

   ofstream fout;
   if(target.lang == "c++"){
      fout.open(boxdir + "main.cpp");
   }else if(target.lang == "c"){
      fout.open(boxdir + "main.c");
   }else if(target.lang == "haskell"){
      fout.open(boxdir + "main.hs");
   }else if(target.lang == "python2"){
      fout.open(boxdir + "main.py");
   }else{
      return CE;
   }
   fout << target.code << flush;
   fout.close();

   if(target.problem_type == 2){
      sout.str("");
      sout << boxdir << "lib" << setw(4) << setfill('0') << target.problem_id << ".h";
      fout.open(sout.str());
      fout << target.interlib << flush;
      fout.close();
   }

   sout.str("");
   if(target.lang == "c++"){
      sout << "/usr/bin/env g++ ./main.cpp -o ./main.out -O2 -w ";
   }else if(target.lang == "c"){
      if(target.std == "" || target.std == "c90"){
         sout << "/usr/bin/env gcc ./main.c -o ./main.out -O2 -ansi -lm -w ";
      }else{
         sout << "/usr/bin/env gcc ./main.c -o ./main.out -O2 -lm -w ";
      }
   }else if(target.lang == "haskell"){
      sout << "/usr/bin/env ghc ./main.hs -o ./main.out -O -tmpdir . -w ";
   }else if(target.lang == "python2"){
      sout << "/usr/bin/env python2 -m py_compile main.py";
   }
   if(!target.std.empty() && target.std != "c90"){
      sout << "-std=" << target.std << " ";
   }
   string comm(sout.str());
   sandboxOptions opt;
   //opt.dirs.push_back("/usr/lib");
   if (target.lang == "haskell") opt.dirs.push_back("/var");
   opt.procs = 50;
   opt.preserve_env = true;
   opt.errout = "compile_error";
   opt.output = "/dev/null";
   opt.timeout = 30 * 1000;
   opt.meta = "./testzone/metacomp";
   opt.fsize_limit = 10 * 1024;

   sandboxExec(boxid, opt, comm);
   string compiled_target;
   if(target.lang == "python2")
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
      sout.str("");
      sout << "/tmp/box/" << spBoxid << "/box/";
      string spBoxdir = sout.str();

      fout.open(spBoxdir + "sj.cpp");
      fout << target.sjcode << flush;
      fout.close();

      sout.str("");
      sout << "/usr/bin/env g++ ./sj.cpp -o ./sj.out -O2 -std=c++11 ";

      opt.meta = "./testzone/metacompsj";

      sandboxExec(spBoxid, opt, sout.str());
      if(access((spBoxdir+"sj.out").c_str(), F_OK) == -1){
         return ER;
      }
   }

   return OK;
}
