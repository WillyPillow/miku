#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<string>
#include<sstream>
#include<iomanip>
#include"sandbox.h"
#include"utils.h"

using namespace std;

int sandboxExec(int boxid, const sandboxOptions &opt, const string &comm)
{
   ostringstream sout;
   sout << "isolate --box-id=" << boxid;
   if(opt.cgroup) sout << " --cg";
   if(opt.preserve_env) {
     sout << " --full-env";
   } else {
     for (const string& env : opt.envs)
       sout<< " --env=" << env;
   }
   for(const string& dir : opt.dirs)
      sout << " --dir=" << dir;
   if(!opt.input.empty()) sout << " --stdin=" << opt.input;
   if(!opt.output.empty()) sout << " --stdout=" << opt.output;
   if(!opt.errout.empty()) sout << " --stderr=" << opt.errout;
   if(!opt.meta.empty()) sout << " --meta=" << opt.meta;
   if(opt.mem != 0) sout << " --mem=" << opt.mem;
   sout << " --processes=" << opt.procs;
   double timeout = opt.timeout * 0.001;
   if(opt.timeout != 0){
      sout << " --time=" << fixed << setprecision(3) << timeout;
      sout << " --wall-time=" << fixed << setprecision(3) << timeout * 2;
   }
   if(opt.fsize_limit) sout << " --fsize=" << opt.fsize_limit;
   sout << " --file-limit=" << opt.file_limit;
   sout << " --extra-time=0.2";
   sout << " --run -- " << comm;
   Log("[debug] box-", boxid, " execute command : ", sout.str());

   system(sout.str().c_str());
   return 0;
}

int sandboxInit(int boxid)
{
   ostringstream sout;
   sout << "isolate --box-id=" << boxid;
   sout << " --cg";
   sout << " --init 2>/dev/null >/dev/null";
   Log("[debug] box-", boxid, " inited");
   system(sout.str().c_str());
   return 0;
}

int sandboxDele(int boxid)
{
   ostringstream sout;
   sout << "isolate --box-id=" << boxid;
   sout << " --cg";
   sout << " --cleanup 2>/dev/null >/dev/null";
   Log("[debug] box-", boxid, " cleaned");
   system(sout.str().c_str());
   return 0;
}
