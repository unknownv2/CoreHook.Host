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

    host_startup_info_t startup_info;
    arguments_t arguments;

    PCWSTR dotnet_sdk_root
        = L"%programfiles%\\dotnet\\sdk\\2.2.100";

    WCHAR dotnet_root[MAX_PATH];
    ::ExpandEnvironmentStringsW(dotnet_sdk_root, dotnet_root, MAX_PATH);

    pal::string_t library_path = dotnet_assembly_name;
    EXPECT_TRUE(pal::realpath(&library_path));

    startup_info.dotnet_root = dotnet_root;
    arguments.managed_application = library_path;
    arguments.app_root = get_directory(arguments.managed_application);

    auto error = corehost::initialize_clr(
        arguments,
        startup_info,
        host_mode_t::muxer);

    EXPECT_EQ(0, error);
    typedef int (STDMETHODCALLTYPE calculator_method_del)(const int a, const int b);

    // Test the 'int Add(int a, int b) => a + b;' method
    calculator_method_del *fn_delegate = nullptr;
    error = corehost::create_delegate(
        assembly_name,
        type_name,
        method_name_add,
        (void**)&fn_delegate
    );
    EXPECT_EQ(0, error);
    const int integer_a = 1;
    const int integer_b = 2;
    const int integer_add_result = integer_a + integer_b;

    EXPECT_EQ(integer_add_result, fn_delegate(integer_a, integer_b));

    // Test the 'int Subtract(int a, int b) => b - a;' method
    error = corehost::create_delegate(
        assembly_name,
        type_name,
        method_name_subtract,
        (void**)&fn_delegate
    );
    EXPECT_EQ(0, error);
    const int integer_subtract_result = integer_b - integer_a;

    EXPECT_EQ(integer_subtract_result, fn_delegate(integer_a, integer_b));
}