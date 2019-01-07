#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>

#ifndef CORELOAD_STRINGIFY
#define CORELOAD_STRINGIFY(x)    CORELOAD_STRINGIFY_(x)
#define CORELOAD_STRINGIFY_(x)    #x
#endif

#define VER_CORELOAD_BITS    CORELOAD_STRINGIFY(CORELOAD_BITS)
