#include "utils.h"

const std::string kBoxRoot = "/tmp/box";
const std::string kTestdataRoot = "./testdata";

std::string PadInt(int x, size_t width) {
  std::string ret = std::to_string(x);
  if (ret.size() < width) ret = std::string(width - ret.size(), '0') + ret;
  return ret;
}

std::string BoxPath(int box) {
  return kBoxRoot + '/' + std::to_string(box) + "/box/";
}
std::string BoxInput(int box) {
  return kBoxRoot + '/' + std::to_string(box) + "/box/input";
}
std::string BoxOutput(int box) {
  return kBoxRoot + '/' + std::to_string(box) + "/box/output";
}
std::string TdPath(int prob) {
  return kTestdataRoot + '/' + PadInt(prob, 4) + '/';
}
std::string TdInput(int prob, int td) {
  return kTestdataRoot + '/' + PadInt(prob, 4) + "/input" + PadInt(td, 3);
}
std::string TdOutput(int prob, int td) {
  return kTestdataRoot + '/' + PadInt(prob, 4) + "/output" + PadInt(td, 3);
}
