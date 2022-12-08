/*
 * Copyright (C) 2013  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <io.h>
#include <stdio.h>
#include <windows.h>

#include "pread.h"

static CRITICAL_SECTION preadsc;

/* http://stackoverflow.com/a/2390626/1392778 */

#ifdef _MSC_VER
#pragma section(".CRT$XCU", read)
#define INITIALIZER2_(f, p)                                                    \
    static void __cdecl f(void);                                               \
    __declspec(allocate(".CRT$XCU")) void(__cdecl * f##_)(void) = f;           \
    __pragma(comment(linker, "/include:" p #f "_")) static void __cdecl f(void)
#ifdef _WIN64
#define INITIALIZER(f) INITIALIZER2_(f, "")
#else
#define INITIALIZER(f) INITIALIZER2_(f, "_")
#endif
#elif defined(__GNUC__)
#define INITIALIZER(f)                                                         \
    static void f(void) __attribute__((constructor));                          \
    static void f(void)
#endif

#ifdef _WIN64
int pread(int fd, void *buf, unsigned int nbyte, __int64 offset) {
    int cc = -1;
    __int64 prev = (__int64)-1L;

    EnterCriticalSection(&preadsc);
    prev = _lseeki64(fd, 0L, SEEK_CUR);
    if (prev == (__int64)-1L) {
        goto done;
    }
    if (_lseeki64(fd, offset, SEEK_SET) != offset) {
        goto done;
    }
    cc = _read(fd, buf, nbyte);

done:
    if (prev != (__int64)-1L) {
        (void)_lseeki64(fd, prev, SEEK_SET);
    }
    LeaveCriticalSection(&preadsc);

    return cc;
}
#else
int pread(int fd, void *buf, unsigned int nbyte, long offset) {
    int cc = -1;
    long prev = -1L;

    EnterCriticalSection(&preadsc);
    prev = _lseek(fd, 0L, SEEK_CUR);
    if (prev == -1L) {
        goto done;
    }
    if (_lseek(fd, offset, SEEK_SET) != offset) {
        goto done;
    }
    cc = _read(fd, buf, nbyte);

done:
    if (prev != -1L) {
        (void)_lseek(fd, prev, SEEK_SET);
    }
    LeaveCriticalSection(&preadsc);

    return cc;
}
#endif

static void deinitialize(void) { DeleteCriticalSection(&preadsc); }

INITIALIZER(initialize) {
    InitializeCriticalSection(&preadsc);
    atexit(deinitialize);
}
