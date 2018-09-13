#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include "logger.h"
#include "mscoree.h"
//#include "sstring.h"
//#include <string>

// Function export macro
#define DllExport __declspec(dllexport)

#define CDllExport extern "C" DllExport

#if !defined(MAX_LONGPATH)
#define MAX_LONGPATH   260 /* max. length of full pathname */
#endif

// Utility macro for testing whether or not a flag is set.
#define HAS_FLAG(value, flag) (((value) & (flag)) == (flag))

// Environment variable for setting whether or not to use Server GC.
// Off by default.
static const wchar_t *serverGcVar = W("CORECLR_SERVER_GC");

// Environment variable for setting whether or not to use Concurrent GC.
// On by default.
static const wchar_t *concurrentGcVar = W("CORECLR_CONCURRENT_GC");

// The name of the CoreCLR native runtime DLL.
static const wchar_t *coreCLRDll = W("CoreCLR.dll");

// The location where CoreCLR is expected to be installed. If CoreCLR.dll isn't
//  found in the same directory as the host, it will be looked for here.
static const wchar_t *coreCLRInstallDirectory = W("%windir%\\system32\\");

#define FunctionNameSize			256
#define AssemblyFunCallArgsSize		512

struct BinaryLoaderArgs
{
	bool Verbose;
	bool WaitForDebugger;
	bool StartAssembly;
	char Reserved[5];
	wchar_t BinaryFilePath[MAX_LONGPATH];
	wchar_t CoreRootPath[MAX_LONGPATH];
	wchar_t CoreLibrariesPath[MAX_LONGPATH];
};

struct AssemblyFunctionCall
{
	wchar_t Assembly[FunctionNameSize];
	wchar_t Class[FunctionNameSize];
	wchar_t Function[FunctionNameSize];
	BYTE	Arguments[AssemblyFunCallArgsSize];
};

struct RemoteFunctionArgs
{
	const BYTE* UserData;
	ULONG UserDataSize;
};

struct RemoteEntryInfo
{
	ULONG HostPID;
	RemoteFunctionArgs Args;
};

ICLRRuntimeHost4 *m_Host;

DWORD m_domainId;

Logger *m_Log;

//static CRITSEC_COOKIE g_pLock = nullptr;

static HRESULT InitializeLock (
	VOID
    );

// Declare DLL exports used for starting and stopping the CoreCLR Runtime
CDllExport
VOID
UnloadRunTime (
	VOID
    );

CDllExport
VOID
ExecuteAssemblyFunction(
	IN CONST AssemblyFunctionCall* args
    );

CDllExport
VOID
LoadAssembly(
	IN CONST BinaryLoaderArgs* args
    );

CDllExport
VOID
ExecuteAssembly(
	IN CONST BinaryLoaderArgs* args
    );

CDllExport
DWORD
StartCLRAndLoadAssembly(
	IN CONST WCHAR*  dllPath,
	IN CONST BOOLEAN verbose,
	IN CONST BOOLEAN waitForDebugger,
	IN CONST WCHAR*  coreRoot,
	IN CONST WCHAR*  coreLibraries,
	IN CONST BOOLEAN executeAssembly
    );

// Encapsulates the environment that CoreCLR will run in, including the TPALIST
class HostEnvironment
{
	// The path to this module
	//PathString m_hostPath;
	std::wstring m_hostPath;
	// The path to the directory containing this module
	//PathString m_hostDirectoryPath;
	std::wstring m_hostDirectoryPath;

	// The name of this module, without the path
	const wchar_t *m_hostExeName;

	// The list of paths to the assemblies that will be trusted by CoreCLR
	//SString m_tpaList;
	std::wstring m_tpaList;
	ICLRRuntimeHost4* m_CLRRuntimeHost;

	HMODULE m_coreCLRModule;

	Logger *m_log;

	DWORD GetModuleFileNameWrapper(
		IN HMODULE hModule,
		std::wstring& buffer
	   )
	{
		WCHAR buf[MAX_PATH];
		DWORD size = MAX_PATH;
		DWORD ret = GetModuleFileNameW(
			hModule,
			buf,
			size
		);
		if (ret == 0)
		{
			return GetLastError();
		}
		buffer.assign(buf);
		return ret;
	}

	// Attempts to load CoreCLR.dll from the given directory.
	// On success pins the dll, sets m_coreCLRDirectoryPath and returns the HMODULE.
	// On failure returns nullptr.
	HMODULE TryLoadCoreCLR(const std::wstring& directoryPath) {

		//StackSString coreCLRPath(directoryPath);
		std::wstring coreCLRPath(directoryPath);

		coreCLRPath.append(coreCLRDll);

		*m_log << W("Attempting to load: ") << coreCLRPath << Logger::endl;

		HMODULE result = LoadLibraryExW(coreCLRPath.c_str(), NULL, 0);
		if (!result) {
			*m_log << W("Failed to load: ") << coreCLRPath << Logger::endl;
			*m_log << W("Error code: ") << GetLastError() << Logger::endl;
			return nullptr;
		}

		// Pin the module - CoreCLR.dll does not support being unloaded.
		HMODULE dummy_coreCLRModule;
		if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, coreCLRPath.c_str(), &dummy_coreCLRModule)) {
			*m_log << W("Failed to pin: ") << coreCLRPath << Logger::endl;
			return nullptr;
		}

		std::wstring coreCLRLoadedPath;
		GetModuleFileNameWrapper(result, coreCLRLoadedPath);

		*m_log << W("Loaded: ") << coreCLRLoadedPath << Logger::endl;

		return result;
	}

public:
	// The path to the directory that CoreCLR is in
	std::wstring m_coreCLRDirectoryPath;

	HostEnvironment(Logger *logger, const wchar_t * coreRootPath)
		: m_log(logger), m_CLRRuntimeHost(nullptr) {

		// Discover the path to this exe's module. All other files are expected to be in the same directory.
		GetModuleFileNameWrapper(::GetModuleHandleW(nullptr), m_hostPath);

		// Search for the last backslash in the host path.
		std::size_t lastBackslash = m_hostPath.find_last_of(W('\\'));

		//SString::CIterator lastBackslash = m_hostPath.End();
		//m_hostPath.FindBack(lastBackslash, W('\\'));

		// Copy the directory path
		//m_hostDirectoryPath.Set(m_hostPath, m_hostPath.Begin(), lastBackslash + 1);
		m_hostDirectoryPath = m_hostPath.substr(0, lastBackslash);
		// Save the exe name
		//m_hostExeName = m_hostPath.GetUnicode(lastBackslash + 1);
		m_hostExeName = m_hostPath.substr(lastBackslash).c_str();

		*m_log << W("Host directory: ") << m_hostDirectoryPath << Logger::endl;

		// Check for %CORE_ROOT% and try to load CoreCLR.dll from it if it is set
		std::wstring coreRoot;

		m_coreCLRModule = NULL; // Initialize this here since we don't call TryLoadCoreCLR if CORE_ROOT is unset.

		if (coreRootPath)
		{
			coreRoot.append(coreRootPath);
			coreRoot.append(W("\\"));
			m_coreCLRModule = TryLoadCoreCLR(coreRoot);
		}
		else
		{
			*m_log << W("CORE_ROOT not set; skipping") << Logger::endl;
			*m_log << W("You can set the environment variable CORE_ROOT to point to the path") << Logger::endl;
			*m_log << W("where CoreCLR.dll lives to help CoreRun.exe find it.") << Logger::endl;
		}

		// Try to load CoreCLR from the directory that coreRun is in
		if (!m_coreCLRModule) {
			m_coreCLRModule = TryLoadCoreCLR(m_hostDirectoryPath);
		}

		if (!m_coreCLRModule) {

			// Failed to load. Try to load from the well-known location.
			wchar_t coreCLRInstallPath[MAX_PATH];
			::ExpandEnvironmentStringsW(coreCLRInstallDirectory, coreCLRInstallPath, MAX_PATH);
			m_coreCLRModule = TryLoadCoreCLR(coreCLRInstallPath);

		}

		if (m_coreCLRModule) {

			// Save the directory that CoreCLR was found in
			DWORD modulePathLength = GetModuleFileNameWrapper(m_coreCLRModule, m_coreCLRDirectoryPath);

			// Search for the last backslash and terminate it there to keep just the directory path with trailing slash
			//SString::Iterator lastBackslash = m_coreCLRDirectoryPath.End();
			//m_coreCLRDirectoryPath.FindBack(lastBackslash, W('\\'));
			//m_coreCLRDirectoryPath.Truncate(lastBackslash + 1);
			std::size_t lastBackslash = m_coreCLRDirectoryPath.find_last_of(W('\\'));
			m_coreCLRDirectoryPath.resize(lastBackslash);

		}
		else {
			*m_log << W("Unable to load ") << coreCLRDll << Logger::endl;
		}
	}

	~HostEnvironment() {
		if (m_coreCLRModule) {
			// Free the module. This is done for completeness, but in fact CoreCLR.dll 
			// was pinned earlier so this call won't actually free it. The pinning is
			// done because CoreCLR does not support unloading.
			::FreeLibrary(m_coreCLRModule);
		}
	}

	bool TPAListContainsFile(
		_In_z_ wchar_t* fileNameWithoutExtension,
		_In_reads_(countExtensions) const wchar_t** rgTPAExtensions, 
		int countExtensions)
	{
		if (m_tpaList.empty()) return false;

		for (int iExtension = 0; iExtension < countExtensions; iExtension++)
		{
			std::wstring fileName;
			fileName.append(W("\\")); // So that we don't match other files that end with the current file name
			fileName.append(fileNameWithoutExtension);
			fileName.append(rgTPAExtensions[iExtension] + 1);
			fileName.append(W(";")); // So that we don't match other files that begin with the current file name

			if (m_tpaList.find(fileName) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	void
	RemoveExtensionAndNi(
		_In_z_ wchar_t* fileName
	    )
	{
		// Remove extension, if it exists
		wchar_t* extension = wcsrchr(fileName, W('.'));
		if (extension != NULL)
		{
			extension[0] = W('\0');

			// Check for .ni
			size_t len = wcslen(fileName);
			if (len > 3 &&
				fileName[len - 1] == W('i') &&
				fileName[len - 2] == W('n') &&
				fileName[len - 3] == W('.'))
			{
				fileName[len - 3] = W('\0');
			}
		}
	}

	void AddFilesFromDirectoryToTPAList(
		_In_z_ const std::wstring& targetPath,
		_In_reads_(countExtensions) const wchar_t** rgTPAExtensions,
		int countExtensions)
	{
		*m_log << W("Adding assemblies from ") << targetPath << W(" to the TPA list") << Logger::endl;
		std::wstring assemblyPath;
		const size_t dirLength = targetPath.size();

		for (int iExtension = 0; iExtension < countExtensions; iExtension++)
		{
			assemblyPath.assign(targetPath);
			assemblyPath.append(rgTPAExtensions[iExtension]);
			WIN32_FIND_DATA data;
			HANDLE findHandle = FindFirstFileW(assemblyPath.c_str(), &data);

			if (findHandle != INVALID_HANDLE_VALUE) {
				do {
					if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						// It seems that CoreCLR doesn't always use the first instance of an assembly on the TPA list (ni's may be preferred
						// over il, even if they appear later). So, only include the first instance of a simple assembly name to allow
						// users the opportunity to override Framework assemblies by placing dlls in %CORE_LIBRARIES%

						// ToLower for case-insensitive comparisons
						wchar_t* fileNameChar = data.cFileName;
						while (*fileNameChar)
						{
							*fileNameChar = towlower(*fileNameChar);
							fileNameChar++;
						}

						// Remove extension
						wchar_t fileNameWithoutExtension[MAX_PATH];
						wcscpy_s(fileNameWithoutExtension, MAX_PATH, data.cFileName);

						RemoveExtensionAndNi(fileNameWithoutExtension);

						// Add to the list if not already on it
						if (!TPAListContainsFile(fileNameWithoutExtension, rgTPAExtensions, countExtensions))
						{
							assemblyPath.resize(dirLength);
							assemblyPath.append(data.cFileName);
							m_tpaList.append(assemblyPath);
							m_tpaList.append(W(";"));
						}
						else
						{
							*m_log << W("Not adding ")
								<< targetPath 
								<< data.cFileName
								<< W(" to the TPA list because another file with the same name is already present on the list")
								<< Logger::endl;
						}
					}
				} while (0 != FindNextFileW(findHandle, &data));

				FindClose(findHandle);
			}
		}
	}

	// Returns the semicolon-separated list of paths to runtime dlls that are considered trusted.
	// On first call, scans the coreclr directory for dlls and adds them all to the list.
	const wchar_t * GetTpaList(const wchar_t * coreLibsPath) {
		if (m_tpaList.empty()) {
			const wchar_t *rgTPAExtensions[] = {
						// Probe for .ni.dll first so that it's preferred
						// if ni and il coexist in the same dir
						W("*.ni.dll"),		
						W("*.dll"),
						W("*.ni.exe"),
						W("*.exe"),
						W("*.ni.winmd"),
						W("*.winmd")
			};

			// Add files from %CORE_LIBRARIES% if specified
			std::wstring coreLibraries;
			if (coreLibsPath) {
				coreLibraries.append(coreLibsPath);
				coreLibraries.append(W("\\"));
				AddFilesFromDirectoryToTPAList(coreLibraries, rgTPAExtensions, _countof(rgTPAExtensions));
			}
			if (coreLibraries.compare(m_coreCLRDirectoryPath) != 0) {
				AddFilesFromDirectoryToTPAList(m_coreCLRDirectoryPath, rgTPAExtensions, _countof(rgTPAExtensions));
			}
		}

		return m_tpaList.c_str();
	}

	// Returns the path to the host module
	const wchar_t * GetHostPath() {
		return m_hostPath.c_str();
	}

	// Returns the path to the host module
	const wchar_t * GetHostExeName() {
		return m_hostExeName;
	}

	// Returns the ICLRRuntimeHost4 instance, loading it from CoreCLR.dll if necessary, or nullptr on failure.
	ICLRRuntimeHost4* GetCLRRuntimeHost() {
		if (!m_CLRRuntimeHost) {

			if (!m_coreCLRModule) {
				*m_log << W("Unable to load ") << coreCLRDll << Logger::endl;
				return nullptr;
			}

			*m_log << W("Finding GetCLRRuntimeHost(...)") << Logger::endl;

			FnGetCLRRuntimeHost pfnGetCLRRuntimeHost =
				(FnGetCLRRuntimeHost)::GetProcAddress(m_coreCLRModule, "GetCLRRuntimeHost");

			if (!pfnGetCLRRuntimeHost) {
				*m_log << W("Failed to find function GetCLRRuntimeHost in ") << coreCLRDll << Logger::endl;
				return nullptr;
			}

			*m_log << W("Calling GetCLRRuntimeHost(...)") << Logger::endl;

			HRESULT hr = pfnGetCLRRuntimeHost(IID_ICLRRuntimeHost4, (IUnknown**)&m_CLRRuntimeHost);
			if (FAILED(hr)) {
				*m_log 
					<< W("Failed to get ICLRRuntimeHost4 interface. ERRORCODE: ")
					<< Logger::hresult
					<< hr
					<< Logger::endl;
				return nullptr;
			}
		}

		return m_CLRRuntimeHost;
	}
};

VOID
SetGlobalHost (
	ICLRRuntimeHost4* host
    )
{
	m_Host = host;
}

ICLRRuntimeHost4*
GetGlobalHost (
	VOID
    )
{
	return m_Host;
}

VOID
SetDomainId (
	IN CONST DWORD domainId
    )
{
	m_domainId = domainId;
}

DWORD
GetDomainId (
	VOID
    )
{
	return m_domainId;
}

VOID
SetLogger (
	IN Logger* log
    )
{
	if (m_Log == NULL) {
		m_Log = log;
	}
}

Logger*
GetLogger (
	VOID
    )
{
	if (m_Log == NULL) {
		m_Log = new Logger();
	}
	return m_Log;
}

VOID 
DeleteLogger (
	VOID
    )
{
	if (m_Log != nullptr) {
		delete m_Log;
	}
}

VOID
RtlLongLongToAsciiHex (
	IN CONST LONGLONG InValue,
	IN CHAR* InBuffer
    )
{
	ULONG           Index;
	ULONG           iChar;
	WCHAR           c;

	for (Index = 0, iChar = 0; Index < 64; Index += 4, iChar++)
	{
#ifdef _M_X64
		c = ((LONGLONG)InValue >> Index) & 0x0F;
#else
		if (Index < 32)
			c = (char)(((LONG)InValue >> Index) & 0x0F);
		else
			c = 0;
#endif

		if (c < 10)
			c += '0';
		else
			c += 'A' - 10;

		InBuffer[15 - iChar] = (char)c;
	}

	InBuffer[16] = 0;
}

INT32
PrintModules (
	VOID
    ) 
{
	HMODULE hMods[1024];
	HANDLE hProcess;
	DWORD cbNeeded;
	unsigned int i;
	DWORD processID = GetCurrentProcessId();
	// Print the process identifier.

	printf("\nProcess ID: %u\n", processID);

	// Get a handle to the process.

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID);
	if (NULL == hProcess)
		return 1;

	// Get a list of all the modules in this process.

	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];

			// Get the full path to the module's file.

			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
				sizeof(szModName) / sizeof(TCHAR)))
			{
				// Print the module name and handle value.

				wprintf(TEXT("\t%s (0x%llX)\n"), szModName, (UINT64)hMods[i]);
			}
		}
	}

	// Release the handle to the process.

	CloseHandle(hProcess);

	return 0;
}
// Creates the startup flags for the runtime, starting with the default startup
// flags and adding or removing from them based on environment variables. Only
// two environment variables are respected right now: serverGcVar, controlling
// Server GC, and concurrentGcVar, controlling Concurrent GC.
STARTUP_FLAGS
CreateStartupFlags (
	VOID
    )
{
	auto initialFlags =
		static_cast<STARTUP_FLAGS>(
			STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_SINGLE_DOMAIN |
			STARTUP_FLAGS::STARTUP_SINGLE_APPDOMAIN |
			STARTUP_FLAGS::STARTUP_CONCURRENT_GC);

	// server GC is off by default, concurrent GC is on by default.
	auto checkVariable = [&](STARTUP_FLAGS flag, const wchar_t *var) {
		wchar_t result[25];
		size_t outsize;
		if (_wgetenv_s(&outsize, result, 25, var) == 0 && outsize > 0) {
			// set the flag if the var is present and set to 1,
			// clear the flag if the var isp resent and set to 0.
			// Otherwise, ignore it.
			if (_wcsicmp(result, W("1")) == 0) {
				initialFlags = static_cast<STARTUP_FLAGS>(initialFlags | flag);
			}
			else if (_wcsicmp(result, W("0")) == 0) {
				initialFlags = static_cast<STARTUP_FLAGS>(initialFlags & ~flag);
			}
		}
	};

	checkVariable(STARTUP_FLAGS::STARTUP_SERVER_GC, serverGcVar);
	checkVariable(STARTUP_FLAGS::STARTUP_CONCURRENT_GC, concurrentGcVar);

	return initialFlags;
}

BOOLEAN
ExecuteAssemblyMain (
	IN CONST INT32 argc,
	IN CONST WCHAR* argv[],
	Logger &log
    )
{
	HRESULT hr;
	DWORD exitCode = -1;
	//CRITSEC_Holder lock(g_pLock);

	const wchar_t* exeName = argc > 0 ? argv[0] : nullptr;
	if (exeName == nullptr)
	{
		log << W("No exename specified.") << Logger::endl;
		return false;
	}

	std::wstring appPath;
	std::wstring managedAssemblyFullName;

	wchar_t* filePart = NULL;

	DWORD size = MAX_PATH;
	wchar_t appPathPtr[MAX_PATH];
	DWORD length = GetFullPathNameW(exeName, size, appPathPtr, &filePart);
	if (length >= size)
	{
		size = length;
		length = GetFullPathNameW(exeName, size, appPathPtr, &filePart);
	}
	if (length == 0 || length >= size || filePart == NULL) {
		log << W("Failed to get full path: ") << exeName << Logger::endl;
		log << W("Error code: ") << GetLastError() << Logger::endl;
		return false;
	}

	managedAssemblyFullName.assign(appPathPtr);

	auto host = GetGlobalHost();
	if (host != nullptr) {

		//lock.Release();

		hr = host->ExecuteAssembly(
			GetDomainId(),
			managedAssemblyFullName.c_str(),
			argc - 1,
			(argc - 1) ? &(argv[1]) : NULL,
			&exitCode);

		if (FAILED(hr)) {
			log << W("Failed call to ExecuteAssembly. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
			return false;
		}
	}

	log << W("App exit value = ") << exitCode << Logger::endl;
	return true;
}

// Execute a method from a class located inside a .NET Core Library Assembly
BOOLEAN 
ExecuteAssemblyClassFunction (
	Logger &log,
	IN CONST WCHAR* assembly,
	IN CONST WCHAR* type,
	IN CONST WCHAR* entry,
	IN CONST BYTE*  arguments
    )
{
	HRESULT hr;
	DWORD exitCode = -1, dwWaitResult = -1;
	RemoteEntryInfo EntryInfo;
	void *pfnDelegate = NULL;
	ICLRRuntimeHost4 * host = NULL;
	typedef void (STDMETHODCALLTYPE MainMethodFp)(const VOID* args);

	//CRITSEC_Holder lock(g_pLock);

	host = GetGlobalHost();

	if (host != nullptr) {

		EntryInfo.HostPID = GetCurrentProcessId();

		hr = host->CreateDelegate(
			GetDomainId(),
			assembly, // Target managed assembly
			type, // Target managed type
			entry, // Target entry point (static method)
			(INT_PTR*)&pfnDelegate);

		if (FAILED(hr) || pfnDelegate == NULL)
		{
			log << W("Failed call to CreateDelegate. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
			return false;
		}
		//lock.Release();
		RemoteFunctionArgs * remoteArgs = (RemoteFunctionArgs*)arguments;
		if (remoteArgs != NULL) {
			char ParamString[17];

			// parse entry arguments
			EntryInfo.Args.UserData = remoteArgs->UserData;
			EntryInfo.Args.UserDataSize = remoteArgs->UserDataSize;

			RtlLongLongToAsciiHex((LONGLONG)&EntryInfo, ParamString);

			((MainMethodFp*)pfnDelegate)(ParamString);
		}
		else {
			((MainMethodFp*)pfnDelegate)(NULL);
		}
	}
	log << W("App exit value = ") << exitCode << Logger::endl;

	return true;
}

BOOLEAN
UnloadStopHost (
	Logger &log
    )
{
	HRESULT hr;
	DWORD exitCode = -1;

	//CRITSEC_Holder lock(g_pLock);

	//-------------------------------------------------------------

	// Unload the AppDomain

	ICLRRuntimeHost4 * host = GetGlobalHost();
	if (host != nullptr) {

		log << W("Unloading the AppDomain") << Logger::endl;

		hr = host->UnloadAppDomain2(
			GetDomainId(),
			true,
			(int *)&exitCode);                          // Wait until done

		if (FAILED(hr)) {
			log << W("Failed to unload the AppDomain. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
			return false;
		}

		log << W("App domain unloaded exit value = ") << exitCode << Logger::endl;

		//-------------------------------------------------------------
		//PrintModules();
		// Stop the host

		log << W("Stopping the host") << Logger::endl;

		hr = host->Stop();

		if (FAILED(hr)) {
			log << W("Failed to stop the host. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
			return false;
		}

		//-------------------------------------------------------------

		// Release the reference to the host

		log << W("Releasing ICLRRuntimeHost4") << Logger::endl;

		host->Release();
	}
	SetGlobalHost(nullptr);

	SetDomainId(-1);

	return true;
}

BOOLEAN
LoadStartHost(
	IN CONST INT32 argc,
	IN CONST WCHAR* argv[],
	Logger &log,
	IN CONST BOOLEAN verbose,
	IN CONST BOOLEAN waitForDebugger,
	DWORD &exitCode,
	IN CONST WCHAR* coreRoot,
	IN CONST WCHAR* coreLibraries,
	IN CONST BOOLEAN executeAssembly
    )
{

	// Assume failure
	exitCode = -1;

	HostEnvironment hostEnvironment(&log, coreRoot);

	//-------------------------------------------------------------

	// Find the specified exe. This is done using LoadLibrary so that
	// the OS library search semantics are used to find it.

	const wchar_t* exeName = argc > 0 ? argv[0] : nullptr;
	if (exeName == nullptr)
	{
		log << W("No exename specified.") << Logger::endl;
		return false;
	}

	// The managed application to run should be the first command-line parameter.
	// Subsequent command line parameters will be passed to the managed app later in this host.
	wchar_t targetApp[MAX_PATH];
	GetFullPathNameW(argv[1], MAX_PATH, targetApp, NULL);
	// </Snippet1>

	// Also note the directory the target app is in, as it will be referenced later.
	// The directory is determined by simply truncating the target app's full path
	// at the last path delimiter (\)
	wchar_t targetAppPath[MAX_PATH];
	wcscpy_s(targetAppPath, targetApp);
	size_t i = wcslen(targetAppPath) - 1;
	while (i >= 0 && targetAppPath[i] != L'\\') i--;
	targetAppPath[i] = L'\0';


	//DWORD length = WszGetFullPathName(exeName, size, appPathPtr, &filePart);


	std::wstring appPath = targetAppPath;
	std::wstring appNiPath;
	std::wstring managedAssemblyFullName;
	std::wstring appLocalWinmetadata;


	managedAssemblyFullName.assign(targetApp);


	log << W("Loading: ") << managedAssemblyFullName << Logger::endl;


	appNiPath.assign(appPath);
	appNiPath.append(W("NI"));
	appNiPath.append(W(";"));
	appNiPath.append(appPath);

	// APP_PATHS
	// App paths are directories to probe in for assemblies which are not one of the well-known Framework assemblies
	// included in the TPA list.
	//
	// For this simple sample, we just include the directory the target application is in.
	// More complex hosts may want to also check the current working directory or other
	// locations known to contain application assets.
	wchar_t appPaths[MAX_PATH * 50];

	// Just use the targetApp provided by the user and remove the file name
	wcscpy_s(appPaths, targetAppPath);

	std::wstring managedAssemblyDirectory = managedAssemblyFullName;

	// Search for the last backslash and terminate it there to keep just the directory path with trailing slash

	auto lastBackslash = managedAssemblyDirectory.find_last_of(W("\\"));
	managedAssemblyDirectory.resize(lastBackslash + 1);

	// Construct native search directory paths
	std::wstring nativeDllSearchDirs(appPath);

	nativeDllSearchDirs.append(W(";"));
	nativeDllSearchDirs.append(managedAssemblyDirectory);

	nativeDllSearchDirs.append(W(";"));
	nativeDllSearchDirs.append(coreLibraries);
	nativeDllSearchDirs.append(W(";"));
	nativeDllSearchDirs.append(hostEnvironment.m_coreCLRDirectoryPath);


	// NATIVE_DLL_SEARCH_DIRECTORIES
	// Native dll search directories are paths that the runtime will probe for native DLLs called via PInvoke
	wchar_t nativeDllSearchDirectories[MAX_PATH * 50];
	wcscpy_s(nativeDllSearchDirectories, appPaths);
	wcscat_s(nativeDllSearchDirectories, MAX_PATH * 50, L";");
	wcscat_s(nativeDllSearchDirectories, MAX_PATH * 50, coreRoot);


	// Start the CoreCLR
	//CRITSEC_Holder lock(g_pLock);

	ICLRRuntimeHost4 *host = hostEnvironment.GetCLRRuntimeHost();
	if (!host) {
		log << W("Unable to get ICLRRuntimeHost4 handle") << Logger::endl;
		return false;
	}

	HRESULT hr;

	STARTUP_FLAGS flags = CreateStartupFlags();
	log << W("Setting ICLRRuntimeHost4 startup flags") << Logger::endl;
	log << W("Server GC enabled: ") << HAS_FLAG(flags, STARTUP_FLAGS::STARTUP_SERVER_GC) << Logger::endl;
	log << W("Concurrent GC enabled: ") << HAS_FLAG(flags, STARTUP_FLAGS::STARTUP_CONCURRENT_GC) << Logger::endl;

	// Default startup flags
	hr = host->SetStartupFlags(flags);
	if (FAILED(hr)) {
		log << W("Failed to set startup flags. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
		return false;
	}

	log << W("Starting ICLRRuntimeHost4") << Logger::endl;

	hr = host->Start();
	if (FAILED(hr)) {
		log << W("Failed to start CoreCLR. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
		return false;
	}

	std::wstring  tpaList;
	if (!managedAssemblyFullName.empty())
	{
		// Target assembly should be added to the tpa list. Otherwise corerun.exe
		// may find wrong assembly to execute.
		// Details can be found at https://github.com/dotnet/coreclr/issues/5631
		tpaList = managedAssemblyFullName;
		tpaList.append(W(";"));
	}

	tpaList.append(hostEnvironment.GetTpaList(coreLibraries));

	//-------------------------------------------------------------

	// Create an AppDomain

	// Allowed property names:
	// APPBASE
	// - The base path of the application from which the exe and other assemblies will be loaded
	//
	// TRUSTED_PLATFORM_ASSEMBLIES
	// - The list of complete paths to each of the fully trusted assemblies
	//
	// APP_PATHS
	// - The list of paths which will be probed by the assembly loader
	//
	// APP_NI_PATHS
	// - The list of additional paths that the assembly loader will probe for ngen images
	//
	// NATIVE_DLL_SEARCH_DIRECTORIES
	// - The list of paths that will be probed for native DLLs called by PInvoke
	//
	const wchar_t *property_keys[] = {
		W("TRUSTED_PLATFORM_ASSEMBLIES"),
		W("APP_PATHS"),
		W("APP_NI_PATHS"),
		W("NATIVE_DLL_SEARCH_DIRECTORIES")
	};
	const wchar_t *property_values[] = {
		// TRUSTED_PLATFORM_ASSEMBLIES
		tpaList.c_str(),
		// APP_PATHS
		appPath.c_str(),
		// APP_NI_PATHS
		appNiPath.c_str(),
		// NATIVE_DLL_SEARCH_DIRECTORIES
		nativeDllSearchDirs.c_str()
	};

	log << W("Creating an AppDomain") << Logger::endl;
	for (int idx = 0; idx < sizeof(property_keys) / sizeof(wchar_t*); idx++)
	{
		log << property_keys[idx] << W("=") << property_values[idx] << Logger::endl;
	}

	DWORD domainId;

	hr = host->CreateAppDomainWithManager(
		hostEnvironment.GetHostExeName(),   // The friendly name of the AppDomain
		// Flags:
		// APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS
		// - By default CoreCLR only allows platform neutral assembly to be run. To allow
		//   assemblies marked as platform specific, include this flag
		//
		// APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP
		// - Allows sandboxed applications to make P/Invoke calls and use COM interop
		//
		// APPDOMAIN_SECURITY_SANDBOXED
		// - Enables sandboxing. If not set, the app is considered full trust
		//
		// APPDOMAIN_IGNORE_UNHANDLED_EXCEPTION
		// - Prevents the application from being torn down if a managed exception is unhandled
		//
		APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS |
		APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP |
		APPDOMAIN_DISABLE_TRANSPARENCY_ENFORCEMENT,
		NULL,                // Name of the assembly that contains the AppDomainManager implementation
		NULL,                    // The AppDomainManager implementation type name
		sizeof(property_keys) / sizeof(wchar_t*),  // The number of properties
		property_keys,
		property_values,
		&domainId);


	if (FAILED(hr)) {

		log << W("Failed call to CreateAppDomainWithManager. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
		return false;
	}

	if (waitForDebugger)
	{
		if (!IsDebuggerPresent())
		{
			log << W("Waiting for the debugger to attach. Press any key to continue ...") << Logger::endl;
			getchar();
			if (IsDebuggerPresent())
			{
				log << "Debugger is attached." << Logger::endl;
			}
			else
			{
				log << "Debugger failed to attach." << Logger::endl;
			}
		}
	}

	if (executeAssembly) {

		hr = host->ExecuteAssembly(
			domainId,
			managedAssemblyFullName.c_str(),
			argc - 1,
			(argc - 1) ? &(argv[1]) : NULL,
			&exitCode);

		if (FAILED(hr)) {

			log << W("Failed call to ExecuteAssembly. ERRORCODE: ") << Logger::hresult << hr << Logger::endl;
			return false;
		}

		log << W("App exit value = ") << exitCode << Logger::endl;
	}
	else {
		SetGlobalHost(host);

		SetDomainId(domainId);
	}

	return true;
}

DWORD
ValidateArgument (
	IN CONST WCHAR* argument,
	IN DWORD maxSize
    )
{
	if (argument != nullptr) {
		const size_t dirLength = wcslen(argument);
		if (dirLength >= maxSize) {
			return E_INVALIDARG;
		}
	}
	else {
		return E_INVALIDARG;
	}
	return S_OK;
}

DWORD
ValidateAssemblyFunctionCallArgs (
	IN CONST AssemblyFunctionCall* args
    ) 
{
	if (args != nullptr) {
		if (SUCCEEDED(ValidateArgument(args->Assembly, FunctionNameSize))
			&& SUCCEEDED(ValidateArgument(args->Class, FunctionNameSize))
			&& SUCCEEDED(ValidateArgument(args->Function, FunctionNameSize))) {
			return S_OK;
		}
	}
	return E_INVALIDARG;
}
DWORD 
ValidateBinaryLoaderArgs (
	IN CONST BinaryLoaderArgs* args
    )
{
	if (args != nullptr) {
		if (SUCCEEDED(ValidateArgument(args->BinaryFilePath, MAX_LONGPATH))
			&& SUCCEEDED(ValidateArgument(args->CoreRootPath, MAX_LONGPATH))
			&& SUCCEEDED(ValidateArgument(args->CoreLibrariesPath, MAX_LONGPATH))) {
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

// Load a .NET Core DLL Application or Library into the Host Application and also execute it if desired
CDllExport
DWORD
StartCLRAndLoadAssembly (
	IN CONST WCHAR*  dllPath,
	IN CONST BOOLEAN verbose,
	IN CONST BOOLEAN waitForDebugger,
	IN CONST WCHAR*  coreRoot,
	IN CONST WCHAR*  coreLibraries,
	IN CONST BOOLEAN executeAssembly
    )
{

	// Parse the options from the command line
	DWORD exitCode = -1;
	if (SUCCEEDED(ValidateArgument(dllPath, MAX_LONGPATH))
		&& SUCCEEDED(ValidateArgument(coreRoot, MAX_LONGPATH))
		&& SUCCEEDED(ValidateArgument(coreLibraries, MAX_LONGPATH))) {

		Logger* log = GetLogger();
		if (verbose) {
			log->Enable();
		}
		else {
			log->Disable();
		}

		const wchar_t * params[] = {
			dllPath
		};
		DWORD paramCount = 1;

		const bool success =
			LoadStartHost(
			  paramCount,
			  params,
			  *log,
			  verbose,
			  waitForDebugger,
			  exitCode, 
			  coreRoot,
			  coreLibraries,
			  executeAssembly);

		*log << W("Execution ") << (success ? W("succeeded") : W("failed")) << Logger::endl;
	}
	return exitCode;
}

CDllExport
VOID
ExecuteAssembly (
	IN CONST BinaryLoaderArgs* args
    )
{
	if (SUCCEEDED(ValidateBinaryLoaderArgs(args))) {

		StartCLRAndLoadAssembly(
			args->BinaryFilePath,
			args->Verbose, 
			args->WaitForDebugger,
			args->CoreRootPath, 
			args->CoreLibrariesPath,
			true);
	}
}

CDllExport
VOID
LoadAssembly (
	IN CONST BinaryLoaderArgs* args
    )
{
	if (SUCCEEDED(ValidateBinaryLoaderArgs(args))) {
		StartCLRAndLoadAssembly(
			args->BinaryFilePath,
			args->Verbose,
			args->WaitForDebugger,
			args->CoreRootPath,
			args->CoreLibrariesPath,
			args->StartAssembly);
	}
}

CDllExport
VOID
ExecuteAssemblyFunction (
	IN CONST AssemblyFunctionCall* args
    )
{
	if (SUCCEEDED(ValidateAssemblyFunctionCallArgs(args))) {

		ExecuteAssemblyClassFunction(*GetLogger(),
			args->Assembly,
			args->Class,
			args->Function,
			args->Arguments);
	}
}

CDllExport
VOID
UnloadRunTime(
	VOID
    ) 
{
	UnloadStopHost(*GetLogger());
}

static 
HRESULT
InitializeLock(
	VOID
    )
{

	return S_OK;
}

BOOLEAN 
WINAPI
DllMain(
	IN HINSTANCE hDllHandle,
	IN DWORD     nReason,
	IN LPVOID    Reserved
)
{
	BOOLEAN bSuccess = TRUE;
	switch (nReason)
	{
		case DLL_PROCESS_ATTACH:
		{
		}
		break;
		case DLL_PROCESS_DETACH:
		{

			DeleteLogger();
		}
		break;
	}

	return bSuccess;
}
