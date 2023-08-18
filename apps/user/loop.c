#include "app.h"
#include <stdlib.h>

// The same as clock.c, except this one does not print
int main(int argc, char** argv) {
    int cnt = (argc == 1)? 1000 : atoi(argv[1]);

    for (int i = 0; i < cnt; i++) {
        for (int j = 0; j < 5000000; j++);
        //printf("clock: tick#%d / #%d\r\n", i + 1, cnt);
    }

    return 0;
}