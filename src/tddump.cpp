#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <sstream>
#include <unistd.h>
#include <iomanip>

#include "server_io.h"
#include "utils.h"

using namespace std;

bool enable_log;

int main(int argc, char *argv[]) {
  if(argc < 2) return 0;
  std::ios::sync_with_stdio(false);
  std::cerr << std::nounitbuf;
  int pid = cast(argv[1]).to<int>();
  submission sub;
  sub.problem_id = pid;

  string testdata_dir = TdPath(sub.problem_id);
  if (access(testdata_dir.c_str(), F_OK)) Execute("mkdir", "-p", testdata_dir);
  downloadTestdata(sub);

  return 0;
}

