#include "pch.h"
#include "coreload.h"

TEST(TestExecuteDotnetAssembly, TestExecuteDotnetAssemblyName) {
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
        = L"%programfiles%\\dotnet\\sdk\\2.2.100";

    WCHAR dotnet_root[MAX_PATH];
    ::ExpandEnvironmentStringsW(dotnet_sdk_root, dotnet_root, MAX_PATH);

    pal::string_t library_path = dotnet_assembly_name;
    EXPECT_TRUE(pal::realpath(&library_path));

    core_host_arguments host_arguments = { 0 };
    wcscpy_s(host_arguments.assembly_file_path, MAX_PATH, library_path.c_str());
    wcscpy_s(host_arguments.core_root_path, MAX_PATH, dotnet_root);

    auto error = StartCoreCLR(&host_arguments);
 
    EXPECT_EQ(NO_ERROR, error);
    typedef int (STDMETHODCALLTYPE calculator_method_fn)(const int a, const int b);

    // Test the 'int Add(int a, int b) => a + b;' method
    calculator_method_fn *calculator_delegate = nullptr;
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_add,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    EXPECT_EQ(NO_ERROR, error);
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
    EXPECT_EQ(NO_ERROR, error);
    const int integer_subtract_result = integer_b - integer_a;

    EXPECT_EQ(integer_subtract_result, calculator_delegate(integer_a, integer_b));

    // Test the 'int Multiply(int a, int b) => a * b;' method
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_multiply,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    EXPECT_EQ(NO_ERROR, error);
    const int integer_multiply_result = integer_a * integer_b;

    EXPECT_EQ(integer_multiply_result, calculator_delegate(integer_a, integer_b));

    // Test the 'int Divide(int a, int b) => a / b;' method
    error = CreateAssemblyDelegate(
        assembly_name,
        type_name,
        method_name_divide,
        reinterpret_cast<void**>(&calculator_delegate)
    );
    EXPECT_EQ(NO_ERROR, error);
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

    EXPECT_EQ(NOERROR, error);
    assembly_function_call assembly_function_call = { 0 };

    pal::string_t assembly_name_wide;
    pal::string_t type_name_wide;
    pal::string_t method_name_wide;

    pal::utf8_palstring(assembly_name, &assembly_name_wide);
    pal::utf8_palstring(type_name, &type_name_wide);
    pal::utf8_palstring(method_name_load, &method_name_wide);

    wcscpy_s(assembly_function_call.assembly_name, max_function_name_size, assembly_name_wide.c_str());
    wcscpy_s(assembly_function_call.class_name, max_function_name_size, type_name_wide.c_str());
    wcscpy_s(assembly_function_call.function_name, max_function_name_size, method_name_wide.c_str());

    EXPECT_EQ(NOERROR, ExecuteAssemblyFunction(&assembly_function_call));

    // Unload the AppDomain and stop the host
    EXPECT_EQ(NOERROR, UnloadRuntime());
}