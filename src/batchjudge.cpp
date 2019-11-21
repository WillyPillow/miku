#include <iostream>
#include <algorithm>
#include "sandbox.h"
#include "utils.h"

using namespace std;

int BatchJudge(int pid, int td, int boxid, int tl, int ml, int ol, int testee,
               string lang) {
  ios::sync_with_stdio(false);
  cerr << nounitbuf;

  string target;
  if (lang == "python2" || lang == "python3") {
    target = "main.pyc";
  } else {
    target = "main.out";
  }

  //init box
  sandboxInit(boxid);
  string boxpath = BoxPath(boxid);
  string tdinput = TdInput(pid, td);
  string boxinput = BoxInput(boxid);
  string boxoutput = BoxOutput(boxid);
  int ret = Execute("cp", tdinput, boxinput);
  if (ret) return ret;
  chmod(boxpath.c_str(), 0755);
  chmod(boxinput.c_str(), 0666);
  chmod(boxoutput.c_str(), 0666);
  ret = Execute("cp", BoxPath(testee) + target, boxpath + target);
  if (ret) return ret;

  //set options
  sandboxOptions opt;
  opt.cgroup = true;
  opt.procs = 1;
  opt.input = "input";
  opt.output = "output";
  opt.errout = "/dev/null";
  opt.meta = "./testzone/META" + PadInt(td);
  opt.timeout = tl;
  opt.mem = ml;
  opt.fsize_limit = ol;
  opt.envs.push_back(string("PATH=") + getenv("PATH"));
  opt.file_limit = 48;
  if (lang == "python2" || lang == "python3") {
    opt.envs.push_back("HOME=" + BoxPath(boxid));
    opt.envs.push_back("PYTHONIOENCODING=utf-8");
  }

  //invoke box command
  if (lang == "python2") {
    sandboxExec(boxid, opt, {"/usr/bin/env", "python2.7", "main.pyc"});
  } else if (lang == "python3") {
    sandboxExec(boxid, opt, {"/usr/bin/env", "python3.7", "main.pyc"});
  } else {
    sandboxExec(boxid, opt, {"main.out"});
  }
  return 0;
}

