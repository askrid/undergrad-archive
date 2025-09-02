#include <iostream>

void three_swap(int *a, int *b, int *c);
void three_swap(int &a, int &b, int &c);

int main() {
    using namespace std;

    int a = 1, b = 2, c = 3;
    cout << a << b << c << endl;

    three_swap(&a, &b, &c);
    cout << a << b << c << endl;

    three_swap(a, b, c);
    cout << a << b << c << endl;

    return 0;
}

void three_swap(int *a, int *b, int *c) {
    int tmp;

    tmp = *a;
    *a = *b;
    *b = *c;
    *c = tmp;
}

void three_swap(int &a, int &b, int &c) {
    int tmp;
    
    tmp = a;
    a = b;
    b = c;
    c = tmp;
}