#include <cmath>

int hamming_distance(int x, int y) {
    int res = 0;

    while (x != 0 || y != 0) {
        if ((x & 0b1) != (y & 0b1)) res++;
        x >>= 1;
        y >>= 1;
    }

    return res;
}
