# CoreHook.Host

Dynamic libraries for hosting .NET Core with [CoreHook](https://github.com/unknownv2/CoreHook) in an unmanaged application on Linux, macOS, and Windows.

## Building

First checkout the repository to a folder
```
git clone https://github.com/unknownv2/CoreHook.Host.git
cd CoreHook.Host
```
## Unix

### Linux and macOS (x64)
Make sure you have `clang++` installed, then run:

```
cd Unix
make
```
It will produce a file called `libcorerun.so` (Linux) or `libcorerun.dylib` (macOS), which you will place in the same directory as the output of the build of your program, which is usually a folder called `netcoreapp2.0`.

## Windows (x86, x64, ARM32, ARM64)

### Windows (x86, x64)

Build instructions coming soon... for now, you can download the 32-bit and 64-bit Windows binaries from [here](https://github.com/unknownv2/CoreHook.Host/releases). Extract the `zip` file, then place the `CoreRunDLL32.dll` or `CoreRunDLL64.dll` in the build output directory of your program, which is usually a folder called `netcoreapp2.0`.

### Windows (ARM32, ARM64)

Build instructions coming soon... for now, you can download the ARM32 and ARM64 Windows binaries from [here](https://github.com/unknownv2/CoreHook.Host/releases). Extract the `*-arm.zip` file , then place the `CoreRunDLL32.dll` or `CoreRunDLL64.dll` in the output directory created when you publish a program with `dotnet publish -r win-arm`, which is usually a folder called `publish`.

## References

* [CoreCLR Hosts](https://github.com/dotnet/coreclr/tree/master/src/coreclr/hosts)
* [Using corerun To Run .NET Core Application](https://github.com/dotnet/coreclr/blob/master/Documentation/workflow/UsingCoreRun.md)
* [.NET Core Hosting Sample](https://github.com/dotnet/samples/tree/master/core/hosting)
* [Hosting .NET Core](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)