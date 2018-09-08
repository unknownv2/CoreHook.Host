# CoreHook.Host

Dynamic libraries for hosting .NET Core with [CoreHook](https://github.com/unknownv2/CoreHook) in an unmanaged application on Linux, macOS, and Windows.

## Build status

| Build server | Platform    | Build status                             |
| ------------ | ----------- | ---------------------------------------- |
| AppVeyor     | Windows     | [![Build status](https://ci.appveyor.com/api/projects/status/7c0lfec5c7tlvo2a/branch/master?svg=true)](https://ci.appveyor.com/project/unknownv2/corehook-host/branch/master) |

## Building

First checkout the repository to a folder by running:
```
git clone https://github.com/unknownv2/CoreHook.Host.git
cd CoreHook.Host
```
## Unix (x64, ARM, ARM64)

### Linux and macOS
Make sure you have `clang++` installed, then run:

```
cd Unix
make
```
It will produce a file called `libcorerun.so` (Linux) or `libcorerun.dylib` (macOS), which you will place in the same directory as the output of the build of your program, which is usually a folder called `netcoreapp2.1`.

## Windows (x86, x64, ARM, ARM64)

### Requirements

* Visual Studio 2017

### Instructions
Open the `Visual Studio` solution file in the [Windows](Windows) folder and build your target platform. The built binaries will output in the solution directory as:

| Target    | Folder Name   |
| --------  |:-------------:|
| Win32     | bin.X86       |
| X64       | bin.X64       |
| ARM       | bin.ARM       |
| ARM64     | bin.ARM64     |


#### Releases 
 You can also download the pre-built Windows binaries [here](https://github.com/unknownv2/CoreHook.Host/releases).
 
 For `x86, x64`, extract the `*.zip` file, then place the `corerundll32.dll` or `corerundll64.dll` in the build output directory of your program.
 
 For `ARM, ARM64`, extract the `*-arm.zip` file , then place the `corerundll32.dll` or `corerundll64.dll` in the output directory created when you publish a program with `dotnet publish -r win-arm`, which is usually a folder called `publish`.

## Notes

The `corerundll` project is based on the [CoreCLR](https://github.com/dotnet/coreclr) simple host example. The next major update to the hosting library project will be based on the [core-setup](https://github.com/dotnet/core-setup/) host which supports parsing the `.deps.json` and `runtimeconfig.json` application configuration files.

## References
* [.NET Core Hosting Sample](https://github.com/dotnet/samples/tree/master/core/hosting)
* [CoreCLR Hosts](https://github.com/dotnet/coreclr/tree/master/src/coreclr/hosts)
* [Hosting .NET Core](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
* [Using corerun To Run .NET Core Application](https://github.com/dotnet/coreclr/blob/master/Documentation/workflow/UsingCoreRun.md)