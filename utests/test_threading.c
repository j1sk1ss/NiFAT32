/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    Unit test for the reader-writer lock primitives (std/threading.c).

Dependencies:
    - utest.h - Common unit test wrapper.
*/

#include "utest.h"

int utest_threading_locks() {
    lock_t lock = NULL_LOCK;
    UT_ASSERT(THR_require_read(&lock) == 1);
    UT_ASSERT(THR_require_read(&lock) == 1);
    UT_ASSERT(THR_require_write(&lock, 7) == 0);
    UT_ASSERT(THR_release_read(&lock) == 1);
    UT_ASSERT(THR_release_read(&lock) == 1);
    UT_ASSERT(THR_require_write(&lock, 7) == 1);
    UT_ASSERT(THR_release_write(&lock, 8) == 0);
    UT_ASSERT(THR_release_write(&lock, 7) == 1);
    return 1;
}
