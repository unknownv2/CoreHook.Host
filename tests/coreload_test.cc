#include "pch.h"
#include "coreload.h"

TEST(TestExecuteDotnetAssembly, TestExecuteDotnetAssemblyName) {
    int result = Initialize();
    EXPECT_EQ(0, result);
}