#include <iostream>

int main() {
    using namespace std;

    string name;
    cin >> name;

    if (name == "Youngki") {
        cout << "Hello, Professor!" << endl;
    } else {
        cout << "Hello, " << name << "!" << endl;
    }

    return 0;
}