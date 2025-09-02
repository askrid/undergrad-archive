#include <iostream>
#include <fstream>

bool is_prime(int);

int main() {
    using namespace std;

    ifstream input("./cmake-build-debug/input.txt");
    ofstream output("./cmake-build-debug/output.txt");

    string line;
    int n;
    while (getline(input, line)) {
        n = stoi(line);
        output << n << ": " << is_prime(n) << endl; 
    }

    input.close();
    output.close();

    return 0;
}

bool is_prime(int n) {
     if (n > 2 && n % 2 == 0)
        return false;
    
    for (int i = 3; i*i <= n; i += 2) {
        if (n % i == 0) {
            return false;
        }
    }

    return true;
}