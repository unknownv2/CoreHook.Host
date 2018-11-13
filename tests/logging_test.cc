#include "pch.h"
#include "logging.h"

inline void err_vprintf(const wchar_t* format, va_list vl) {
    ::vfwprintf(stderr, format, vl); ::fputwc(_X('\n'), stderr);
}

void logTest() {
    coreload::logging::logger::instance().trace(_X("Hello %s"), _X("world"));
}
TEST(TestCaseNameLog, TestNameLog) {

   // err_vprintf(_X("Hello %s"), "world");
    
    std::thread threads[0x200];
    for (int x = 0; x < 0x200; x++) {
        threads[x] = std::thread(logTest);
    }
    for (int x = 0; x < 0x200; x++) {
        threads[x].join();
    }
    coreload::logging::logger::instance().trace(_X("Hello %s"), _X("world2"));
    coreload::logging::logger::instance().trace(_X("Hello %s"), _X("world3"));

    
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
}