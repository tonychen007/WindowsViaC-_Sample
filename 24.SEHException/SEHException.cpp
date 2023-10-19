#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <Windows.h>

void TestExecuteHandler();
void TestExecuteException();
void TestGlobalUnwind1();
void TestGlobalUnwind2();
void TestContinueSearch();
void TestRaiseException();

DWORD HandlerFilter(int *p, EXCEPTION_POINTERS* excpt);

int main() {
    printf("TestExecuteHandler\n");
    TestExecuteHandler();

    printf("\n");
    printf("TestExecuteException\n");
    TestExecuteException();

    printf("\n");
    printf("TestGlobalUnwind1\n");
    TestGlobalUnwind1();

    printf("\n");
    printf("TestGlobalUnwind2\n");
    TestGlobalUnwind2();

    printf("\n");
    printf("TestContinueSearch\n");
    TestContinueSearch();

    printf("\n");
    printf("TestRaiseException\n");
    TestRaiseException();
}

void TestExecuteHandler() {
    int v = 0;

    __try {
        int b = 3 / v;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("Exception: divided by zero\n");
    }
}

int gv = 1;
void TestExecuteException() {
    int b = 0;

    __try {
        // only work for global value
        b = 3 / gv;
        printf("Continue execution\n");
    }
    __except (HandlerFilter(0, GetExceptionInformation())) {
    }
}

void TestGlobalUnwind1() {
    __try {
        __try {
            int a = 0;
            int b = 3 / a;
        }
        __finally {
            printf("finally\n");
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("EXCEPTION_EXECUTE_HANDLER\n");
    }
}

void TestGlobalUnwind2() {
    __try {
        __try {
            int a = 0;
            int b = 3 / a;
        }
        __finally {
            printf("finally return\n");
            return;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("EXCEPTION_EXECUTE_HANDLER WILL NEVER CALLED\n");
    }
}

void TestContinueSearch() {
    __try {
        __try {
            int a = 0;
            int b = 3 / a;
        }
        __except(EXCEPTION_CONTINUE_SEARCH) {
            printf("EXCEPTION_CONTINUE_SEARCH WILL NEVER CALLED\n");
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("EXCEPTION_EXECUTE_HANDLER\n");
    }
}

DWORD HandlerFilter(int* p, EXCEPTION_POINTERS* excpt) {
    gv = 1;

    int len = excpt->ExceptionRecord->NumberParameters;
    char** pv = (char**)excpt->ExceptionRecord->ExceptionInformation;
    printf("Exception params is:\n");
    for (int i = 0; i < len; i++) {
        printf("%s---", pv[i]);
    }
    printf("\n");

    return EXCEPTION_CONTINUE_EXECUTION;
}

void TestRaiseException() {
    const char* err[] = { "tony","chen", "error"};

    __try {
        RaiseException(1, 0, _countof(err), (ULONG_PTR*)&err);
    }
    __except (HandlerFilter(0, GetExceptionInformation())) {

    }
}