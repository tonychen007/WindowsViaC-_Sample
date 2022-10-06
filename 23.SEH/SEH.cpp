#include <stdio.h>
#include <Windows.h>

struct Foo {
    ~Foo() {
        printf("~Foo\n");
    }
};

DWORD gVal = 0;

DWORD TestSEHFinally();
DWORD TestSEHFinallyGoto();
DWORD TestSEHFinallyReturn();
void TestErrorInTry();
DWORD FuncaDoodleDoo();

int main() {
    printf("TestSEHFinally\n");
    TestSEHFinally();

    printf("\n");
    printf("TestSEHFinallyGoto\n");
    TestSEHFinallyGoto();

    printf("\n");
    printf("TestSEHFinallyReturn\n");
    gVal = TestSEHFinallyReturn();
    printf("The val is : %d\n", gVal);

    printf("\n");
    printf("TestErrorInTry\n");
    //TestErrorInTry();

    printf("\n");
    printf("FuncaDoodleDoo\n");
    gVal = FuncaDoodleDoo();
    printf("The val is : %d\n", gVal);
}

DWORD TestSEHFinally() {
    __try {
        printf("try\n");
        return 0;
    }
    __finally {        
        printf("finally\n");
    }

    // should never reach here
    printf("return\n");

    return 0;
}

DWORD TestSEHFinallyGoto() {
    __try {
        printf("try\n");
        goto end;
    }
    __finally {
        printf("finally\n");
    }

    // should never reach here
    printf("return\n");

end:
    printf("goto block\n");
    return 0;
}

DWORD TestSEHFinallyReturn() {
    __try {
        printf("try\n");
        return 0;
    }
    __finally {
        printf("finally\n");
        return 99;
    }

    // should never reach here
    printf("return\n");
}

void TestErrorInTry() {
    __try {
        /* finally will not execute in debug mode*/
        *(int*)(0) = 5;
    }
    __finally {
        AbnormalTermination();
        printf("Finally Error\n");
    }
}

DWORD FuncaDoodleDoo() {
    DWORD dwTemp = 0;
    while (dwTemp < 10) {
        __try {
            if (dwTemp == 2)
                continue;
            if (dwTemp == 3)
                break;
        }
        __finally {
            dwTemp++;
            //return 99;
        }
        dwTemp++;
    }
    dwTemp += 10;
    return(dwTemp);
}