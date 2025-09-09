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

  while (!file.eof()) {
    file.read(buffer, sizeof(buffer));
    std::streamsize bytes_read = file.gcount();
    if (bytes_read > 0) {
      data.insert(data.end(), buffer, buffer + bytes_read);
    }
  }

  return picosha2::hash256_hex_string(data);
}


}
