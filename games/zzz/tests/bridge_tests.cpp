// The test target links a test-built copy of the production bridge source.
// Internal fixture assertions are compiled into that same translation unit,
// avoiding an include of a production .cpp from a test executable.
#include <iostream>

extern "C" int zzz_bridge_test_main(int argc, char** argv);

int main(int argc, char** argv) {
    return zzz_bridge_test_main(argc, argv);
}
