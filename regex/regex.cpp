#include <iostream>
#include <string>
#include <regex>

using namespace std;

int main() {
    string fnames[5] = {"foo.txt", "bar.txt", "test", "a0.txt", "AAA.txt"};
    regex txt_regex("[a-z]+\\.txt");
}