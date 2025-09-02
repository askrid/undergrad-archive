#include <iostream>

int main() {
    using namespace std;

    char a[100], b[50];

    cout << "write 1st word:" << endl;
    cin >> a;
    cout << "write 2nd word:" << endl;
    cin >> b;
    
    char* a_ptr = a;
    char* b_ptr = b;

    while(*(++a_ptr));
    while(*(a_ptr++) = *(b_ptr++));

    cout << a << endl;

    return 0;
}