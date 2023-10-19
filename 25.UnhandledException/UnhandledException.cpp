#include <stdio.h>
#include <Windows.h>

LONG UnhandleFilterExecuteHandler(EXCEPTION_POINTERS* ExceptionInfo);
LONG UnhandleFilterContinueException(EXCEPTION_POINTERS* ExceptionInfo);
LONG UnhandleFilterContinueSearch(EXCEPTION_POINTERS* ExceptionInfo);

void TestUnhandleException();

int gv = 0;

int main() {
    //_set_abort_behavior(0, _CALL_REPORTFAULT);

    LPTOP_LEVEL_EXCEPTION_FILTER prevFilter = SetUnhandledExceptionFilter(UnhandleFilterExecuteHandler);
    printf("TestUnhandleException\n");
    TestUnhandleException();

    printf("\n");
    SetUnhandledExceptionFilter(UnhandleFilterContinueException);
    printf("TestUnhandleException\n");
    //TestUnhandleException();

    printf("\n");
    SetUnhandledExceptionFilter(UnhandleFilterContinueSearch);
    printf("TestUnhandleException\n");
    TestUnhandleException();
}

void TestUnhandleException() {
    __try {
        int a = 0;
        int b = 3 / gv;
    }
    __except (EXCEPTION_CONTINUE_SEARCH) {
        printf("after UnhandleFilter\n");
    }

    printf("Continue to run...\n");
}

LONG UnhandleFilterExecuteHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    gv = 1;
    printf("In UnhandleFilter\n");
    printf("The flag is EXCEPTION_EXECUTE_HANDLER, it calls previous unhandleFilter\n");
    printf("But the process will terminate\n");

    return EXCEPTION_EXECUTE_HANDLER;
}

LONG UnhandleFilterContinueException(EXCEPTION_POINTERS* ExceptionInfo) {
    gv = 1;
    printf("In UnhandleFilter\n");
    printf("The flag is EXCEPTION_CONTINUE_EXCEPTION\n");

    return EXCEPTION_CONTINUE_EXECUTION;
}

LONG UnhandleFilterContinueSearch(EXCEPTION_POINTERS* ExceptionInfo) {
    gv = 1;
    printf("In UnhandleFilter\n");
    printf("The flag is EXCEPTION_CONTINUE_SEARCH\n");

    return EXCEPTION_CONTINUE_SEARCH;
}