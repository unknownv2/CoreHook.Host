#ifndef CORELOAD_H
#define CORELOAD_H

#include "pal.h"
#include "utils.h"
#include "logging.h"
#include "deps_format.h"
#include "deps_entry.h"
#include "deps_resolver.h"
#include "fx_muxer.h"
#include "fx_ver.h"
#include "coreclr.h"
#include "status_code.h"
#include "arguments.h"
#include "libhost.h"

int Initialize();

#endif // CORELOAD_H