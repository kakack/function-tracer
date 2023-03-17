#include "json/json.h"
#include <iostream>
#include <fstream>  

/** \brief Write a Value object to a string.
 * Example Usage:
 * $g++ stringWrite.cpp -ljsoncpp -std=c++11 -o stringWrite
 * $./stringWrite
 * {
 *     "action" : "run",
 *     "data" :
 *     {
 *         "number" : 1
 *     }
 * }
 */
using namespace std;
int main() {
  Json::Value root;
  Json::Value data;
  constexpr bool shouldUseOldWay = false;
  root["action"] = "run";
  data["number"] = 1;
  root["data"] = data;
  ofstream os;
  os.open("sample.json", std::ios::out | std::ios::app);
  os << "HelloWorld\n";
  if (shouldUseOldWay) {
    Json::FastWriter writer;
    const std::string json_file = writer.write(root);
    std::cout << json_file << std::endl;
  } else {
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    std::cout << json_file << std::endl;
    os << json_file;
  }
  os.close();
  return EXIT_SUCCESS;
}
