#include<string>
#include<iostream>
#include<cstdio>
#include<sstream>
#include<iomanip>
#include<fstream>
#include<cstdlib>
#include<unistd.h>
#include"server_io.h"

int respondValidating(int submission_id) {
  return Execute("respond_validating.py", PadInt(submission_id));
}

int fetchSubmission(submission &sub) {
   FILE *Pipe = popen("fetch_submission.py", "r");
   fscanf(Pipe, "%d", &sub.submission_id);
   if (sub.submission_id < 0) {
      pclose(Pipe);
      return sub.submission_id;
   }
   fscanf(Pipe, "%d", &sub.problem_id);
   fscanf(Pipe, "%d", &sub.problem_type);
   fscanf(Pipe, "%d", &sub.submitter_id);
   char buff[30];
   fscanf(Pipe, "%s", buff);
   pclose(Pipe);
   if(string(buff) == "c++11"){
      sub.lang = "c++";
      sub.std = "c++11";
   }else if(string(buff) == "c++14"){
      sub.lang = "c++";
      sub.std = "c++14";
   }else if(string(buff) == "c++17"){
      sub.lang = "c++";
      sub.std = "c++17";
   }else if(string(buff) == "c++98"){
      sub.lang = "c++";
      sub.std = "c++98";
   }else if(string(buff) == "c90"){
      sub.lang = "c";
      sub.std = "c90";
   }else if(string(buff) == "c99"){
      sub.lang = "c";
      sub.std = "c99";
   }else if(string(buff) == "c11"){
      sub.lang = "c";
      sub.std = "c11";
   }else if(string(buff) == "haskell"){
      sub.lang = "haskell";
      sub.std = "";
   }else if(string(buff) == "python2"){
      sub.lang = "python2";
      sub.std = "";
   }else if(string(buff) == "python3"){
      sub.lang = "python3";
      sub.std = "";
   }else if(string(buff) == "c++"){
      sub.lang = "c++";
      sub.std = "c++14";
   }else if(string(buff) == "c"){
      sub.lang = "c";
      sub.std = "c11";
   }else{
      sub.lang = "c++";
      sub.std = "c++11";
   }
   return 0;
}

int downloadTestdata(submission &sub) {
   ostringstream sout;
   sout << "./testdata/" << setfill('0') << setw(4) << sub.problem_id;
   sout << "/input";
   string dir(sout.str());

   sout.str("");
   sout << "fetch_testdata_meta.py " << sub.problem_id;
   FILE *Pipe = popen(sout.str().c_str(), "r");
   fscanf(Pipe, "%d", &sub.testdata_count);
   for(int i = 0; i < sub.testdata_count; ++i){
      int testdata_id;
      long long timestamp;
      fscanf(Pipe, "%d %lld", &testdata_id, &timestamp);
      bool flag = true;
      sout.str("");
      sout << dir << setfill('0') << setw(3) << i;
      string td(sout.str());
      ifstream fin(td + ".meta");
      long long ts;
      if(fin >> ts){
         if(ts == timestamp){
            flag = false;
         }
      }
      fin.close();
      if(flag){
         //need to renew td
         sout.str("");
         sout << testdata_id << ' ' << sub.problem_id << ' ' << i;
         Execute("fetch_testdata.py", PadInt(testdata_id),
             PadInt(sub.problem_id), PadInt(i));
         ofstream fout(td + ".meta");
         fout << timestamp << endl;
      }
   }
   pclose(Pipe);
   return 0;
}

int fetchProblem(submission &sub)
{
   //get submission code
   char *buff = new char[5*1024*1024];
   ostringstream sout;
   sout << "fetch_code.py " << sub.submission_id;
   FILE* Pipe = popen(sout.str().c_str(), "r");
   sout.str("");
   while(fgets(buff, 5*1024*1024, Pipe) != NULL)
      sout << buff;
   sub.code = sout.str();
   pclose(Pipe);

   //get sjcode
   sout.str("");
   sout << "fetch_sjcode.py " << sub.problem_id;
   Pipe = popen(sout.str().c_str(), "r");
   sout.str("");
   while(fgets(buff, 5*1024*1024, Pipe) != NULL)
      sout << buff;
   sub.sjcode = sout.str();
   pclose(Pipe);

   //get interlib
   sout.str("");
   sout << "fetch_interlib.py " << sub.problem_id;
   Pipe = popen(sout.str().c_str(), "r");
   sout.str("");
   while(fgets(buff, 5*1024*1024, Pipe) != NULL)
      sout << buff;
   sub.interlib = sout.str();
   pclose(Pipe);

   delete [] buff;
   //check if testdata dir exists
   string testdata_dir = TdPath(sub.problem_id);
   if (access(testdata_dir.c_str(), F_OK)) Execute("mkdir", "-p", testdata_dir);

   //download testdata
   if(downloadTestdata(sub) == -1){
      return -1;
   }

   //get memlimit, timelimit
   sout.str("");
   sout << "fetch_limits.py " << sub.problem_id;
   Pipe = popen(sout.str().c_str(), "r");
   for(int i = 0; i < sub.testdata_count; ++i){
      fscanf(Pipe, "%d %d", &sub.time_limit[i], &sub.mem_limit[i]);
   }
   pclose(Pipe);

   return 0;
}

int sendResult(submission &sub, int verdict, bool done) {
  std::string result;
  if (verdict == CE) {
    result = "CE";
  } else if(verdict == ER) {
    result = "ER";
  } else {
    for (int i = 0; i < sub.testdata_count; ++i) {
      result += fromVerdict(sub.verdict[i]).toAbr() + '/' +
          PadInt(sub.time[i]) + '/' + PadInt(sub.mem[i]) + '/';
      Log("td", i, " : time ", sub.time[i], " mem ", sub.mem[i], " verdict ",
          fromVerdict(sub.verdict[i]).toStr());
    }
  }
  std::string fin = done ? "OK" : "NO";
  Execute("update_verdict.py", PadInt(sub.submission_id), result, fin, fin);
  return 0;
}

int sendMessage(const submission &sub, string message)
{
   ostringstream sout;
   sout << "update_message.py " << sub.submission_id;
   FILE* Pipe = popen(sout.str().c_str(), "w");
   fprintf(Pipe, "%s", message.c_str());
   pclose(Pipe);
   return 0;
}
