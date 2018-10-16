#include "pch.h"
#include "../../src/corerundll/corerundll.h"

TEST(TestCoreCLRHost, TestDotNetAssemblyExecution) {

    PCWSTR dotnetAssemblyName = L"Calculator.dll";
    static const wchar_t *coreCLRInstallDirectory = L"%programfiles%\\dotnet\\shared\\Microsoft.NETCore.App\\2.1.5";

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
    AddMethodFp *pfnAddDelegate = NULL;

    EXPECT_EQ(NO_ERROR,
        CreateAssemblyDelegate(
            L"Calculator",
            L"Calculator.Calculator",
            L"Add",
            reinterpret_cast<PVOID*>(&pfnAddDelegate)));

    const int integerA = 1;
    const int integerB = 2;
    const int integerResult = integerA + integerB;

    EXPECT_EQ(integerResult, pfnAddDelegate(integerA, integerB));
}