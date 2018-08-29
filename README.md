# CoreHook.Host

Dynamic libraries for hosting .NET Core with [CoreHook](https://github.com/unknownv2/CoreHook) in an unmanaged application on Linux, macOS, and Windows.

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

### Windows (x86, x64)

Open the `Visual Studio` solution file in the [Windows](Windows) folder and build for the `Win32` or `x64` platform. The built binaries will be in the `bin.X86` folder for `Win32` and `bin.X64` for `x64`. 

#### Releases 
 You can download the 32-bit and 64-bit Windows binaries [here](https://github.com/unknownv2/CoreHook.Host/releases). Extract the `zip` file, then place the `CoreRunDLL32.dll` or `CoreRunDLL64.dll` in the build output directory of your program, which is usually a folder called `netcoreapp2.1`.

### Windows (ARM, ARM64)

Open the `Visual Studio` solution file in the [Windows](Windows) folder and build for the `ARM` or `ARM64` platform. The built binaries will be in the `bin.ARM` folder for `ARM` and `bin.ARM64` for `ARM64`. 

#### Releases 
You can download the ARM and ARM64 Windows binaries [here](https://github.com/unknownv2/CoreHook.Host/releases). Extract the `*-arm.zip` file , then place the `CoreRunDLL32.dll` or `CoreRunDLL64.dll` in the output directory created when you publish a program with `dotnet publish -r win-arm`, which is usually a folder called `publish`.

## Notes

The `corerundll` project is based on the [CoreCLR](https://github.com/dotnet/coreclr) simple host example. The next major update to the hosting library project will be based on the [core-setup](https://github.com/dotnet/core-setup/) host which supports parsing the `.deps.json` and `runtimeconfig.json` files.

## References
* [.NET Core Hosting Sample](https://github.com/dotnet/samples/tree/master/core/hosting)
* [CoreCLR Hosts](https://github.com/dotnet/coreclr/tree/master/src/coreclr/hosts)
* [Hosting .NET Core](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
* [Using corerun To Run .NET Core Application](https://github.com/dotnet/coreclr/blob/master/Documentation/workflow/UsingCoreRun.md)