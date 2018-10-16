#include "pch.h"
#include "../../src/corerundll/corerundll.h"

TEST(TestCoreCLRHost, TestDotNetAssemblyExecution) {

    // Assembly file name for getting base library path 
    // that is given to the .NET Core host when starting it
    PCWSTR dotnetAssemblyName =     L"Calculator.dll";
    // Assembly name used for delegate resolving
    PCWSTR assemblyName =           L"Calculator";
    // Class name used for delegate resolving
    PCWSTR assemblyType =           L"Calculator.Calculator";
    // Static class function names used for delegate resolving
    PCWSTR assemblyEntryAdd =       L"Add";
    PCWSTR assemblyEntrySubtract =  L"Subtract";
    PCWSTR assemblyEntryMultiply =  L"Multiply";
    PCWSTR assemblyEntryDivide =    L"Divide";

    PCWSTR coreCLRInstallDirectory
        = L"%programfiles%\\dotnet\\shared\\Microsoft.NETCore.App\\2.1.5";

    BinaryLoaderArgs binaryLoaderArgs = { 0 };
    AssemblyFunctionCall assemblyFunctionCall = { 0 };
    binaryLoaderArgs.Verbose = true;
    binaryLoaderArgs.WaitForDebugger = false;
    wcscpy_s(binaryLoaderArgs.BinaryFilePath, MAX_PATH, dotnetAssemblyName);

    WCHAR coreCLRInstallPath[MAX_PATH];
    ::ExpandEnvironmentStringsW(coreCLRInstallDirectory, coreCLRInstallPath, MAX_PATH);

    wcscpy_s(binaryLoaderArgs.CoreRootPath, MAX_PATH, coreCLRInstallPath);

    EXPECT_EQ(NOERROR, StartCoreCLR(&binaryLoaderArgs));

    typedef int (STDMETHODCALLTYPE AddMethodFp)(const int a, const int b);
    AddMethodFp *pfnMethodDelegate = NULL;

    // Test the 'int Add(int a, int b) => a + b;' method
    EXPECT_EQ(NOERROR,
        CreateAssemblyDelegate(
            assemblyName,
            assemblyType,
            assemblyEntryAdd,
            reinterpret_cast<PVOID*>(&pfnMethodDelegate)));

    // Our simple test values
    const int integerA = 1;
    const int integerB = 2;
    const int integerAddResult = integerA + integerB;

    EXPECT_EQ(integerAddResult, pfnMethodDelegate(integerA, integerB));

    // Test the 'int Subtract(int a, int b) => b - a;' method
    pfnMethodDelegate = NULL;

    EXPECT_EQ(NOERROR,
        CreateAssemblyDelegate(
            assemblyName,
            assemblyType,
            assemblyEntrySubtract,
            reinterpret_cast<PVOID*>(&pfnMethodDelegate)));

    const int integerSubResult = integerB - integerA;
    EXPECT_EQ(integerSubResult, pfnMethodDelegate(integerA, integerB));

    // Test the 'int Multiply(int a, int b) => a * b;' method
    pfnMethodDelegate = NULL;

    EXPECT_EQ(NOERROR,
        CreateAssemblyDelegate(
            assemblyName,
            assemblyType,
            assemblyEntryMultiply,
            reinterpret_cast<PVOID*>(&pfnMethodDelegate)));

    const int integerMulResult = integerA * integerB;
    EXPECT_EQ(integerMulResult, pfnMethodDelegate(integerA, integerB));

    // Test the 'int Divide(int a, int b) => a / b;' method
    pfnMethodDelegate = NULL;

    EXPECT_EQ(NOERROR,
        CreateAssemblyDelegate(
            assemblyName,
            assemblyType,
            assemblyEntryDivide,
            reinterpret_cast<PVOID*>(&pfnMethodDelegate)));

    const int integerDivResult = integerA / integerB;
    EXPECT_EQ(integerDivResult, pfnMethodDelegate(integerA, integerB));

    EXPECT_EQ(NOERROR, UnloadRunTime());
}