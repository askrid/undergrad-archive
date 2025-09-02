#include <string>

using namespace std;

bool is_palindrome(std::string s) {
    int len = s.length(), half = len / 2;

    for (int i = 0; i < half; i++) {
        if (s[i] != s[len-1-i]) return false;
    }

    return true;
}
