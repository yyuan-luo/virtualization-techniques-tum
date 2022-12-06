#include <stdlib.h>

int func() {
    int a = 6;
    a = 0;
    a += 3;
    a--;
    return a;
}

int main() {
    func();
}