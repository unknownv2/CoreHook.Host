# CoreHook.Host

[![License](https://img.shields.io/badge/License-MIT-blue.svg?style=flat-square)](https://github.com/unknownv2/CoreHook.Host/blob/master/LICENSE)
[![Releases](https://img.shields.io/github/release/unknownv2/CoreHook.Host.svg?colorB=33b2e0&style=flat-square
)](https://github.com/unknownv2/CoreHook.Host/releases)

A library for hosting .NET Core with [CoreHook](https://github.com/unknownv2/CoreHook) in an unmanaged application on Windows.

## Build status

| Build server | Platform    | Build status                             |
| ------------ | ----------- | ---------------------------------------- |
| AppVeyor     | Windows     | [![Build status](https://ci.appveyor.com/api/projects/status/7c0lfec5c7tlvo2a/branch/master?style=flat-square)](https://ci.appveyor.com/project/unknownv2/corehook-host/branch/master) |

## Building

## Windows (x86, x64, ARM, ARM64)

### Requirements

* Visual Studio

### Visual Studio

Building the DLL requires Visual Studio and there are two options: You can build the DLL by using the `Visual Studio IDE` or using `msbuild` within the `Developer Command Prompt` (the solution file is for `Visual Studio 2017`, for other versions see the [CMake section](#cmake)).

You can choose a configuration (**Debug|Release**) and a platform (**X86|X64|ARM|ARM64**) and build. 

An example for building the X64 `coreload64.dll` in the `Release` configuration:

```
msbuild build/msvc/coreload/coreload-dll.vcxproj /p:Configuration=Release /p:Platform=x64
```

To build the entire solution (which also builds the testing project), you can run:

```
nuget restore build/msvc/coreload.sln
msbuild build/msvc/coreload.sln /p:Configuration=Release /p:Platform=x64
```

The build output DLL will be inside the `bin` folder.

### CMake

You can also build the library using CMake. You can run the `build.cmd` file to build for the `x86` and `x64` architectures using `Visual Studio 2017`. CMake also gives you the option to build with an older version of `Visual Studio` such as `2015` or `2013`.

### Tests

You can compile the .NET class [`Calculator.cs`](tests/dotnet/Calculator.cs), which is required for the tests, with the command:

```
csc -target:library Calculator.cs
```

### Binary Releases 
 You can also download the pre-built Windows binaries [here](https://github.com/unknownv2/CoreHook.Host/releases).
 
 For `x86, x64`, extract the zip corresponding to your target architecture, then place the `coreload32.dll` or `coreload64.dll` in the build output directory of your program.
 
 For `ARM, ARM64`,  extract the zip corresponding to your target architecture, then place the `coreload32.dll` or `coreload64.dll` in the output directory of your published program, created either from using the [Publishing Script](https://github.com/unknownv2/CoreHook#publishing-script) or the `dotnet publish` command.

## Credits

The `coreload` project is based on the [core-setup](https://github.com/dotnet/core-setup/) host which supports parsing the `.deps.json` and `runtimeconfig.json` application configuration files.

## References
* [.NET Core Hosting Sample](https://github.com/dotnet/samples/tree/master/core/hosting)
* [CoreCLR Hosts](https://github.com/dotnet/coreclr/tree/master/src/coreclr/hosts)
* [Hosting .NET Core](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
* [Using corerun To Run .NET Core Application](https://github.com/dotnet/coreclr/blob/master/Documentation/workflow/UsingCoreRun.md)