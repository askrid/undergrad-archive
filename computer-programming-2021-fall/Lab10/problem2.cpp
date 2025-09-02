#include <iostream>

#define PI 3.14159
#define AREA(r) PI*r*r

int main() {
    using namespace std;

    int r;
    cin >> r;
    cout << AREA(r) << endl;

    return 0;
}