#include "util.h"

int log2_floor(unsigned int x) {
    int res = -1;
    while (x > 0) {
        x >>= 1;
        res++;
    }
    return res;
}
