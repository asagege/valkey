/**
 * wrappers.h - Function Wrapper Declarations for GoogleTest Unit Tests
 *
 * PURPOSE:
 * This file declares C function wrappers that enable mocking of Valkey C functions
 * in GoogleTest unit tests. It bridges C code with gtest infrastructure.
 *
 * HOW IT WORKS:
 * 1. Declare wrapper functions with __wrap_ prefix (e.g., __wrap_mstime for mstime())
 * 2. generate-wrappers.py parses this file and auto-generates TWO files:
 *    - generated_wrappers.hpp (MockValkey class with MOCK_METHOD macros)
 *    - generated_wrappers.cpp (wrapper implementations that delegate to MockValkey)
 * 3. Build system uses --wrap linker flags to redirect calls: mstime() -> __wrap_mstime()
 * 4. GoogleTest can mock these wrappers to control behavior and verify calls
 *
 * RULES:
 * - All wrapper functions MUST be prefixed with __wrap_
 * - Function signatures MUST exactly match the original C function
 * - DO NOT wrap variadic functions (functions with ...) - GoogleTest doesn't support them
 * - Each wrapper becomes mockable in gtest via the auto-generated MockValkey class
 *
 * WORKFLOW:
 * wrappers.h -> generate-wrappers.py -> [generated_wrappers.hpp + generated_wrappers.cpp]
 *                                        -> linked with gtest
 *
 * See: wrapper_util.py, generate-wrappers.py
 */

#include <sched.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WRAPPERS_H
#define __WRAPPERS_H
// C/C++ cross-compatibility definitions:
// Some C keywords or built-in types (e.g., _Atomic, _Bool) are not
// recognized or have different meanings in C++. To allow C headers
// to be included in C++ code without errors, we redefine them appropriately.
#define _Atomic              /* _Atomic is not a C++ keyword; define empty */
#define _Bool bool           /* Replace C _Bool with C++ bool */
#define typename _typename   /* Avoid conflict with C++ 'typename' keyword */
#define protected protected_ /* Avoid conflict with C++ 'protected' keyword */

#include "ae.h"
#include "allocator_defrag.h"
#include "config.h"
#include "connection.h"
#include "crc64.h"
#include "crccombine.h"
#include "crcspeed.h"
#include "dict.h"
#include "endianconv.h"
#include "entry.h"
#include "expire.h"
#include "fifo.h"
#include "fmacros.h"
#include "hashtable.h"
#include "intset.h"
#include "kvstore.h"
#include "listpack.h"
#include "monotonic.h"
#include "mt19937-64.h"
#include "mutexqueue.h"
#include "quicklist.h"
#include "rax.h"
#include "sds.h"
#include "sdsalloc.h"
#include "server.h"
#include "sha1.h"
#include "sha256.h"
#include "util.h"
#include "valkey_strtod.h"
#include "vector.h"
#include "vset.h"
#include "ziplist.h"
#include "zipmap.h"
#include "zmalloc.h"
#include "bio.h"
#include "call_reply.h"

/**
 * The list of wrapper methods defined.  Each wrapper method must
 * conform to the same naming conventions (i.e. prefix with a
 * '__wrap_') and have its method signature match the overridden
 * method exactly.
 *
 * Important: Please read the README.md file for guidelines about mocking. Your
 * usage of mocking will not be approved if it doesn't follow the guidelines.
 *
 * Note: You should NOT wrap variable argument functions (i.e have "...")
 *       See: https://github.com/google/googletest/blob/master/googlemock/docs/gmock_faq.md#can-i-mock-a-variadic-function
 *       Example: serverLog(int level, const char *fmt, ...) should NOT be mocked.
 */
long long __wrap_aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds, aeTimeProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc);
#undef protected
#undef _Bool
#undef typename

#endif
#ifdef __cplusplus
}
#endif
