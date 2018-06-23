# CoreHook.Host

Dynamic libraries for hosting .NET Core in an unmanaged application on Linux, macOS, and Windows.

## Building

First checkout the repository to a folder
```
git clone https://github.com/unknownv2/CoreHook.Host
cd CoreHook.Host
```

### Linux and macOS
Make sure you have `clang++` installed, then run:

```
cd Unix
make
```
It will produce a file called `libcorerun.so` (Linux) or `libcorerun.dylib` (macOS), which you will place in the same directory as the output of the build of your program, which is usually a folder called `netcoreapp2.0`.

### Windows

Build instructions coming soon... for now, you can download the Windows binary from [here](https://github.com/unknownv2/CoreHook.Host/releases). Extract the `zip` file, then place the `CoreRunDLL.dll` in the build output directory of your program, which is usually a folder called `netcoreapp2.0`.

## References

* [CoreCLR Hosts](https://github.com/dotnet/coreclr/tree/master/src/coreclr/hosts)
* [Using corerun To Run .NET Core Application](https://github.com/dotnet/coreclr/blob/master/Documentation/workflow/UsingCoreRun.md)
* [.NET Core Hosting Sample](https://github.com/dotnet/samples/tree/master/core/hosting)
* [Hosting .NET Core](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)