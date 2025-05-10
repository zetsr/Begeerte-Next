#pragma once
#include <windows.h>

namespace ErrorHandlerNs {
    void RegisterExceptionFilter();
    LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);
}