#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<string>
#include<sstream>
#include<iomanip>
#include"sandbox.h"
#include"utils.h"

using namespace std;

int sandboxExec(int boxid, const sandboxOptions &opt, const string &comm) {
  std::vector<std::string> args{"isolate", "--box-id=" + PadInt(boxid)};
  if (opt.cgroup) args.emplace_back("--cg");
  if (opt.preserve_env) {
    args.emplace_back("--full-env");
  } else {
    for (const string& env : opt.envs) {
      args.emplace_back("--env=" + env);
    }
  }
  for (const string& dir : opt.dirs) args.emplace_back("--dir=" + dir);
  if (!opt.input.empty()) args.emplace_back("--stdin=" + opt.input);
  if (!opt.output.empty()) args.emplace_back("--stdout=" + opt.output);
  if (!opt.errout.empty()) args.emplace_back("--stderr=" + opt.errout);
  if (!opt.meta.empty()) args.emplace_back("--meta=" + opt.meta);
  if (opt.mem != 0) args.emplace_back("--mem=" + PadInt(opt.mem));
  args.emplace_back("--processes=" + PadInt(opt.procs));
  auto TimeStr = [](long x){
    return PadInt(x / 1000) + '.' + PadInt(x % 1000, 3);
  };
  if (opt.timeout != 0) {
    args.emplace_back("--time=" + TimeStr(opt.timeout));
    args.emplace_back("--wall-time=" + TimeStr(opt.timeout * 2));
  }
  if (opt.fsize_limit) args.emplace_back("--fsize=" + PadInt(opt.fsize_limit));
  args.emplace_back("--file-limit=" + PadInt(opt.file_limit));
  args.emplace_back("--extra-time=0.2");
  args.emplace_back("--run");
  args.emplace_back("--");
  {
    size_t pos = 0;
    while (true) {
      size_t nxt = comm.find(' ', pos);
      if (nxt == string::npos) {
        args.emplace_back(comm.substr(pos));
        break;
      } else {
        args.emplace_back(comm.substr(pos, nxt - pos));
        pos = nxt + 1;
        while (pos != comm.size() && comm[pos] == ' ') pos++;
        if (pos == comm.size()) break;
      }
    }
  }
  Log("[debug] box-", boxid, "start exec");
  Execute(args);
  return 0;
}

int sandboxInit(int boxid) {
  Log("[debug] box-", boxid, "start init");
  ExecuteRedir("", "/dev/null", "/dev/null", "isolate",
      "--box-id=" + PadInt(boxid), "--cg", "--init");
  return 0;
}

int sandboxDele(int boxid) {
  Log("[debug] box-", boxid, "start clean");
  ExecuteRedir("", "/dev/null", "/dev/null", "isolate",
      "--box-id=" + PadInt(boxid), "--cg", "--cleanup");
  return 0;
}
