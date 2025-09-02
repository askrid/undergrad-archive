bool is_prime(int);

int main() {
    is_prime(66);

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