#include <iostream>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include "sandbox.h"
#include "utils.h"

using namespace std;

bool enable_log;

int main(int argc, char *argv[]) {
  if (argc < 10) {
    std::cerr << "batchjudge: Command line parsing error" << std::endl;
    return -1;
  }
  int problem_id, td, boxid, time_limit, mem_limit, output_limit, testee;
  string lang;
  try {
    problem_id = std::stoi(argv[1]);
    td = std::stoi(argv[2]);
    boxid = std::stoi(argv[3]);
    time_limit = std::stoi(argv[4]);
    mem_limit = std::stoi(argv[5]);
    output_limit = std::stoi(argv[6]);
    testee = std::stoi(argv[7]);
    lang = argv[8];
    enable_log = argv[9][0] == '1';
  } catch (...) {
    for (int i = 1; i < argc; i++) std::cerr << argv[i] << std::endl;
    std::cerr << "batchjudge: Command line parsing error" << std::endl;
    return -1;
  }
  std::ios::sync_with_stdio(false);
  std::cerr << std::nounitbuf;

  string target;
  if (lang == "python2" || lang == "python3") {
    target = "main.pyc";
  } else {
    target = "main.out";
  }

  //init box
  sandboxInit(boxid);
  string boxpath = BoxPath(boxid);
  string tdinput = TdInput(problem_id, td);
  string boxinput = BoxInput(boxid);
  string boxoutput = BoxOutput(boxid);
  Execute("cp", tdinput, boxinput);
  chmod(boxpath.c_str(), 0755);
  //Execute("touch", boxinput); // ??
  //Execute("touch", boxoutput);
  chmod(boxinput.c_str(), 0666);
  chmod(boxoutput.c_str(), 0666);
  Execute("cp", BoxPath(testee) + target, boxpath + target);

  //set options
  sandboxOptions opt;
  opt.cgroup = true;
  opt.procs = 1;
  opt.input = "input";
  opt.output = "output";
  opt.errout = "/dev/null";
  opt.meta = "./testzone/META" + PadInt(td);
  opt.timeout = time_limit;
  opt.mem = mem_limit;
  opt.fsize_limit = output_limit;
  opt.envs.push_back(string("PATH=") + getenv("PATH"));
  if (lang == "python2" || lang == "python3") {
    opt.file_limit = 16;
    opt.envs.push_back("HOME=" + BoxPath(boxid));
  } else {
    opt.file_limit = 10;
  }

  //invoke box command
  if (lang == "python2") {
    sandboxExec(boxid, opt, {"/usr/bin/env", "python2.7", "main.pyc"});
  } else if (lang == "python3") {
    sandboxExec(boxid, opt, {"/usr/bin/env", "python3.6", "main.pyc"});
  } else {
    sandboxExec(boxid, opt, {"main.out"});
  }
  return 0;
}

