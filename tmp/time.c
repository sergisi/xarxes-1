#include<stdio.h>

int time(int iter, int n, int m);
int main() {
    int i;
    for(i=0; i<8; i++) {
        printf("%i\n", time(i, 3, 4));
    }
    return 0;
}

int time(int i, int n, int m) {
    if(i < n) {
        return 1;
    } else if(i-n+2 < m) {
        return (i-n+2);
    } else {
        return m;
    }
}
