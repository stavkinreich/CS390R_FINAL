/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

#include "utils/BaseUtil.h"

/*
Temp allocator is meant for allocating temporary values that don't need
to outlive this stack frame.
It's an alternative to using various AutoFree* classes.

It's a very fast bump allocator.

You must periodically call ResetTempAllocator()
to free memory used by allocator.
A safe place to call it is inside message windows loop.
*/

thread_local static PoolAllocator* gTempAllocator = nullptr;

// forbid inlinining to not blow out the size of callers
NO_INLINE Allocator* GetTempAllocator() {
    if (gTempAllocator) {
        return gTempAllocator;
    }

    gTempAllocator = new PoolAllocator();
    // this can be large because 64k is nothing and it's used frequently
    gTempAllocator->minBlockSize = 64 * 1024;
    return gTempAllocator;
}

void DestroyTempAllocator() {
    delete gTempAllocator;
    gTempAllocator = nullptr;
}

void ResetTempAllocator() {
    if (gTempAllocator) {
        gTempAllocator->Reset(true);
    }
}

namespace str {
TempStr DupTemp(const char* s, size_t cb) {
    return str::Dup(GetTempAllocator(), s, cb);
}

TempWstr DupTemp(const WCHAR* s, size_t cch) {
    return str::Dup(GetTempAllocator(), s, cch);
}

TempStr JoinTemp(const char* s1, const char* s2, const char* s3) {
    return Join(GetTempAllocator(), s1, s2, s3);
}

TempWstr JoinTemp(const WCHAR* s1, const WCHAR* s2, const WCHAR* s3) {
    return Join(GetTempAllocator(), s1, s2, s3);
}

TempStr FormatTemp(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char* res = FmtV(fmt, args);
    va_end(args);
    TempStr tmp = DupTemp(res);
    str::Free(res);
    return tmp;
}

TempStr ReplaceTemp(const char* s, const char* toReplace, const char* replaceWith) {
    char* res = str::Replace(s, toReplace, replaceWith);
    TempStr tmp = str::DupTemp(res);
    str::Free(res);
    return tmp;
}

} // namespace str

TempStr ToUtf8Temp(const WCHAR* s, size_t cch) {
    if (!s) {
        CrashIf((int)cch > 0);
        return nullptr;
    }
    return strconv::WstrToUtf8(s, cch, GetTempAllocator());
}

TempWstr ToWstrTemp(const char* s, size_t cb) {
    if (!s) {
        CrashIf((int)cb > 0);
        return nullptr;
    }
    return strconv::Utf8ToWstr(s, cb, GetTempAllocator());
}
