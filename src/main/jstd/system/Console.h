
#ifndef JSTD_SYSTEM_CONSOLE_H
#define JSTD_SYSTEM_CONSOLE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/internal/NonCopyable.h"

#include "jstd/system/getchar.h"
#include "jstd/test/CPUWarmUp.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER
#include <conio.h>
#endif // _MSC_VER

namespace jstd {

class Console : public internal::NonCopyable
{
public:
    Console() = default;
    ~Console() = default;

    static void Write(const char * fmt, ...) {
        va_list arg_list;
        va_start(arg_list, fmt);
        vprintf(fmt, arg_list);
        va_end(arg_list);
    }

    static void WriteLine(const char * fmt = nullptr, ...) {
        va_list arg_list;
        if (fmt != nullptr) {
            va_start(arg_list, fmt);
            vprintf(fmt, arg_list);
            va_end(arg_list);
        }
        printf("\n");
    }

    static int ReadKey(bool displayTips = true, bool echoInput = false,
                       bool newLine = false, bool enabledCpuWarmup = false) {
        int keyCode;
        if (displayTips) {
            printf("Press any key to continue ...");

            keyCode = jstd_getch();
            printf("\n");
        }
        else {
            keyCode = jstd_getch();
            if (echoInput) {
                if (keyCode != EOF)
                    printf("%c", (char)keyCode);
                else
                    printf("EOF: (%d)", keyCode);
            }
        }

        if (newLine) {
            printf("\n");
        }

        // After call jstd_getch(), warm up the CPU again, at least 500 ms.
        if (enabledCpuWarmup) {
            jtest::CPU::warmup(800);
        }
        return keyCode;
    }

    static int ReadKeyLine(bool displayTips = true, bool echoInput = false,
                           bool newLine = false, bool enabledCpuWarmup = false) {
        int keyCode = ReadKey(displayTips, echoInput, true, false);
        if (newLine) {
            printf("\n");
        }
        return keyCode;
    }

    static int Read() {
        int keyCode = 0;
        // TODO: Read the inputs.
        return keyCode;
    }
};

} // namespace jstd

#endif // JSTD_SYSTEM_CONSOLE_H
