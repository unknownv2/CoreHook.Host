#include "pch.h"
#include "coreload.h"

TEST(ExecuteDotnetAssemblyTest, CanExecuteDotnetAssembly)
{
    // Assembly file name for getting base library path 
    // that is given to the .NET Core host when starting it
    PCWSTR dotnet_assembly_name = L"Calculator.dll";
    // Assembly name used for delegate resolving
    PCSTR assembly_name = "Calculator";
    // Class name used for delegate resolving
    PCSTR type_name = "Calculator.Calculator";
    // Static class function names used for delegate resolving
    PCSTR method_name_add = "Add";
    PCSTR method_name_subtract = "Subtract";
    PCSTR method_name_multiply = "Multiply";
    PCSTR method_name_divide = "Divide";
    PCSTR method_name_load = "Load";

    PCWSTR dotnet_sdk_root
        = L"%programfiles%\\dotnet\\sdk\\2.2.103";

    WCHAR dotnet_root[MAX_PATH];
    ExpandEnvironmentStringsW(dotnet_sdk_root, dotnet_root, MAX_PATH);

    coreload::pal::string_t library_path = dotnet_assembly_name;
    ASSERT_TRUE(coreload::pal::realpath(&library_path));

    core_host_arguments host_arguments = { 0 };
    // The path to the assembly being loaded by CoreCLR.
    wcscpy_s(host_arguments.assembly_file_path, MAX_PATH, library_path.c_str());
    // The path that should contain the runtime configuration or dependencies for the assembly
    wcscpy_s(host_arguments.core_root_path, MAX_PATH, dotnet_root);

    // This will initialize CoreCLR in the current application,
    // which allows us to load assemblies from the paths defined by the
    // runtime configuration or from the base application path of the 
    // test assembly library Calculator.dll.
    auto error = StartCoreCLR(&host_arguments);
 
    ASSERT_EQ(NO_ERROR, error);
    typedef int (STDMETHODCALLTYPE calculator_method_fn)(const int a, const int b);

    // Test the 'int Add(int a, int b) => a + b;' method
    calculator_method_fn *calculator_delegate = nullptr;
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_add,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    ASSERT_EQ(NO_ERROR, error);
    ASSERT_NE(nullptr, calculator_delegate);

    const int integer_a = 1;
    const int integer_b = 2;
    const int integer_add_result = integer_a + integer_b;

    EXPECT_EQ(integer_add_result, calculator_delegate(integer_a, integer_b));

    // Test the 'int Subtract(int a, int b) => b - a;' method
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_subtract,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    ASSERT_EQ(NO_ERROR, error);
    ASSERT_NE(nullptr, calculator_delegate);

    const int integer_subtract_result = integer_b - integer_a;

    EXPECT_EQ(integer_subtract_result, calculator_delegate(integer_a, integer_b));

    // Test the 'int Multiply(int a, int b) => a * b;' method
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_multiply,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    ASSERT_EQ(NO_ERROR, error);
    ASSERT_NE(nullptr, calculator_delegate);

    const int integer_multiply_result = integer_a * integer_b;

    EXPECT_EQ(integer_multiply_result, calculator_delegate(integer_a, integer_b));

    // Test the 'int Divide(int a, int b) => a / b;' method
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_divide,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    ASSERT_EQ(NO_ERROR, error);
    ASSERT_NE(nullptr, calculator_delegate);
    const int integer_divide_result = integer_a / integer_b;

    EXPECT_EQ(integer_divide_result, calculator_delegate(integer_a, integer_b));

    // Test the 'int Load(IntPtr remoteParameters)' method
    typedef void (STDMETHODCALLTYPE load_method_fn)(const void* args);
    load_method_fn* fn_method_load_delegate = nullptr;
    error = CreateAssemblyDelegate(
            assembly_name,
            type_name,
            method_name_load,
            reinterpret_cast<void**>(&fn_method_load_delegate));

    ASSERT_EQ(NOERROR, error);
    ASSERT_NE(nullptr, fn_method_load_delegate);

    assembly_function_call assembly_function_call = { 0 };

    coreload::pal::string_t assembly_name_wide;
    coreload::pal::string_t type_name_wide;
    coreload::pal::string_t method_name_wide;

    coreload::pal::utf8_palstring(assembly_name, &assembly_name_wide);
    coreload::pal::utf8_palstring(type_name, &type_name_wide);
    coreload::pal::utf8_palstring(method_name_load, &method_name_wide);

    wcscpy_s(assembly_function_call.assembly_name, max_function_name_size, assembly_name_wide.c_str());
    wcscpy_s(assembly_function_call.class_name, max_function_name_size, type_name_wide.c_str());
    wcscpy_s(assembly_function_call.function_name, max_function_name_size, method_name_wide.c_str());

    EXPECT_EQ(NOERROR, ExecuteAssemblyFunction(&assembly_function_call));

    // Unload the AppDomain and stop the host
    EXPECT_EQ(NOERROR, UnloadRuntime());
}

TEST(LibraryExportsTest, TestExecuteAssemblyFunctionWithOneEmptyAssemblyName)
{
    assembly_function_call assembly_function_call = { 0 };

    coreload::pal::string_t assembly_name_wide;
    coreload::pal::string_t type_name_wide;
    coreload::pal::string_t method_name_wide;

    const auto empty_string = _X("");

    wcscpy_s(assembly_function_call.assembly_name, max_function_name_size, empty_string);
    wcscpy_s(assembly_function_call.class_name, max_function_name_size, _X("ClassName"));
    wcscpy_s(assembly_function_call.function_name, max_function_name_size, _X("MethodName"));

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, ExecuteAssemblyFunction(&assembly_function_call));
}

TEST(LibraryExportsTest, TestExecuteAssemblyFunctionWithOneEmptyClassName)
{
    assembly_function_call assembly_function_call = { 0 };

    coreload::pal::string_t assembly_name_wide;
    coreload::pal::string_t type_name_wide;
    coreload::pal::string_t method_name_wide;

    const auto empty_string = _X("");

    wcscpy_s(assembly_function_call.assembly_name, max_function_name_size, _X("AssemblyName"));
    wcscpy_s(assembly_function_call.class_name, max_function_name_size, empty_string);
    wcscpy_s(assembly_function_call.function_name, max_function_name_size, _X("MethodName"));

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, ExecuteAssemblyFunction(&assembly_function_call));
}

TEST(LibraryExportsTest, TestExecuteAssemblyFunctionWithOneEmptyClassMethodName)
{
    assembly_function_call assembly_function_call = { 0 };

    coreload::pal::string_t assembly_name_wide;
    coreload::pal::string_t type_name_wide;
    coreload::pal::string_t method_name_wide;

    const auto empty_string = _X("");

    wcscpy_s(assembly_function_call.assembly_name, max_function_name_size, _X("AssemblyName"));
    wcscpy_s(assembly_function_call.class_name, max_function_name_size, _X("ClassName"));
    wcscpy_s(assembly_function_call.function_name, max_function_name_size, empty_string);

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, ExecuteAssemblyFunction(&assembly_function_call));
}

TEST(LibraryExportsTest, TestExecuteAssemblyFunctionWithEmptyArguments)
{
    assembly_function_call assembly_function_call = { 0 };

    coreload::pal::string_t assembly_name_wide;
    coreload::pal::string_t type_name_wide;
    coreload::pal::string_t method_name_wide;

    const auto empty_string = _X("");

    wcscpy_s(assembly_function_call.assembly_name, max_function_name_size, empty_string);
    wcscpy_s(assembly_function_call.class_name, max_function_name_size, empty_string);
    wcscpy_s(assembly_function_call.function_name, max_function_name_size, empty_string);

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, ExecuteAssemblyFunction(&assembly_function_call));
}

TEST(LibraryExportsTest, TestStartCoreCLRWithEmptyArguments)
{
    core_host_arguments host_arguments = { 0 };

    const auto empty_string = _X("");
    wcscpy_s(host_arguments.assembly_file_path, MAX_PATH, empty_string);
    wcscpy_s(host_arguments.core_root_path, MAX_PATH, empty_string);

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, StartCoreCLR(&host_arguments));
}

TEST(TestLibraryExports, TestStartCoreCLRWithEmptyAssemblyPath)
{
    core_host_arguments host_arguments = { 0 };

    const auto empty_string = _X("");
    wcscpy_s(host_arguments.assembly_file_path, MAX_PATH, empty_string);
    wcscpy_s(host_arguments.core_root_path, MAX_PATH, _X("C:\\"));

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, StartCoreCLR(&host_arguments));
}

TEST(TestLibraryExports, TestStartCoreCLRWithEmptyCoreRootPath)
{
    core_host_arguments host_arguments = { 0 };

    const auto empty_string = _X("");
    wcscpy_s(host_arguments.assembly_file_path, MAX_PATH, _X("C:\\"));
    wcscpy_s(host_arguments.core_root_path, MAX_PATH, empty_string);

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, StartCoreCLR(&host_arguments));
}

TEST(TestLibraryExports, TestExecuteAssemblyFunctionWithNullArguments)
{
    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, ExecuteAssemblyFunction(nullptr));
}

TEST(TestLibraryExports, TestStartCoreCLRWithNullArguments)
{
    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, StartCoreCLR(nullptr));
}

TEST(TestLibraryExports, TestStartCoreCLRWithInvalidAssemblyPath)
{
    core_host_arguments host_arguments = { 0 };

    const auto invalid_file_path = _X("^-.3this_is_not_a_file_path");
    wcscpy_s(host_arguments.assembly_file_path, MAX_PATH, invalid_file_path);
    wcscpy_s(host_arguments.core_root_path, MAX_PATH, _X("C:\\"));

    EXPECT_EQ(coreload::StatusCode::InvalidArgFailure, StartCoreCLR(&host_arguments));
}
