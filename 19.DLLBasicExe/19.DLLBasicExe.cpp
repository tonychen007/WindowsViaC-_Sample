#include <stdio.h>
#include "../19.DLLBasic/dll_test.h"

int main() {
    printf("add(1, 2) is %d\n", add(1, 2));
    printf("gVal from DLL is: %d\n", gVal);
}
