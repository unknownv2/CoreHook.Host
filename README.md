# CoreHook.Host

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/unknownv2/CoreHook.Host/blob/master/LICENSE)

A library for hosting .NET Core with [CoreHook](https://github.com/unknownv2/CoreHook) in an unmanaged application on Windows.

## Build status

| Build server | Platform    | Build status                             |
| ------------ | ----------- | ---------------------------------------- |
| AppVeyor     | Windows     | [![Build status](https://ci.appveyor.com/api/projects/status/7c0lfec5c7tlvo2a/branch/master?svg=true)](https://ci.appveyor.com/project/unknownv2/corehook-host/branch/master) |

## Building

## Windows (x86, x64, ARM, ARM64)

### Requirements

* Visual Studio 2017

### Visual Studio

Building the DLL requires Visual Studio and there are two options: You can build the DLL by using the `Visual Studio IDE` or using `msbuild` within the `Developer Command Prompt` (it has been tested with `Visual Studio 2017` only).

You can choose a configuration (**Debug|Release**) and a platform (**X86|X64|ARM|ARM64**) and build. 

An example for building the X64 `corerundll64.dll` in the Release configuration:

```
msbuild Windows/corerundll.sln /p:Configuration=Release /p:Platform=x64
```

The build output DLL will be inside the `Windows` folder.

### CMake

You can also build the library using CMake. For example, to build for the `X86` and `X64` architectures:

```
cd Windows
mkdir build32
mkdir build64
cd build32
cmake -G "Visual Studio 15 2017" ../
cd ../
cd build64
cmake -G "Visual Studio 15 2017 Win64" ../
cd ../
cmake --build build32 --config Debug
cmake --build build32 --config Release
cmake --build build64 --config Debug
cmake --build build64 --config Release
```


### Binary Releases 
 You can also download the pre-built Windows binaries [here](https://github.com/unknownv2/CoreHook.Host/releases).
 
 For `x86, x64`, extract the zip corresponding to your target architecture, then place the `corerundll32.dll` or `corerundll64.dll` in the build output directory of your program.
 
 For `ARM, ARM64`,  extract the zip corresponding to your target architecture, then place the `corerundll32.dll` or `corerundll64.dll` in the output directory of your published program, created either from using the [Publishing Script](https://github.com/unknownv2/CoreHook#publishing-script) or the `dotnet publish` command.

## Notes

The `corerundll` project is based on the [CoreCLR](https://github.com/dotnet/coreclr) simple host example. The next major update to the hosting library project will be based on the [core-setup](https://github.com/dotnet/core-setup/) host which supports parsing the `.deps.json` and `runtimeconfig.json` application configuration files.

## References
* [.NET Core Hosting Sample](https://github.com/dotnet/samples/tree/master/core/hosting)
* [CoreCLR Hosts](https://github.com/dotnet/coreclr/tree/master/src/coreclr/hosts)
* [Hosting .NET Core](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
* [Using corerun To Run .NET Core Application](https://github.com/dotnet/coreclr/blob/master/Documentation/workflow/UsingCoreRun.md)