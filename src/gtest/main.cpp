#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <cstring>
extern "C" {
#include "util.h"
}


bool accurate = false;
bool large_memory = false;
bool valgrind = false;
char *seed = nullptr;

bool hasFlag(int argc, char **argv, const char *flag) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

char *getFlagValue(int argc, char **argv, const char *flag) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return nullptr;
}

int main(int argc, char **argv) {
    accurate = hasFlag(argc, argv, "--accurate");
    large_memory = hasFlag(argc, argv, "--large-memory");
    valgrind = hasFlag(argc, argv, "--valgrind");
    seed = getFlagValue(argc, argv, "--seed");
    if (seed) {
        srandom(static_cast<unsigned>(atoi(seed)));
        setRandomSeedCString(seed, strlen(seed));
    }

    // The following line must be executed to initialize GoogleTest before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
