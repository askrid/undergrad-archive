
int* pascal_triangle(int N) {
    int* res = new int[N];
    int half = (N + 1) / 2;
    long prev(1);

    res[0] = res[N-1] = 1;
    for (int i = 1; i < half; i++) {
        prev = prev * (N-i) / i;
        res[i] = res[N-1-i] = prev;
    }

    return res;
}
