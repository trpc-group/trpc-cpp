#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include "picosha2.h"  // 下载自 https://github.com/okdshin/PicoSHA2

namespace http::demo {

std::string CalculateSHA256(const std::string& file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) return "";

  std::vector<unsigned char> data;
  char buffer[8192];
  while (file.read(buffer, sizeof(buffer))) {
    data.insert(data.end(), buffer, buffer + file.gcount());
  }
  if (file.gcount() > 0) {
    data.insert(data.end(), buffer, buffer + file.gcount());
  }

  return picosha2::hash256_hex_string(data);
}

// 你也可以直接在 Post() 方法中调用该函数来校验上传结果
}
