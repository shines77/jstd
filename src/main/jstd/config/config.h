
#ifndef JSTD_CONFIG_CONFIG_H
#define JSTD_CONFIG_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/config/config_pre.h"
#include "jstd/config/config_arch.h"
//#include "jstd/config/config_hw.h"

#if defined(__linux__) || defined(__LINUX__) || defined(__apple__)
#include "jstd/config/linux/config.h"
#elif defined(_WIN32) || defined(_MSC_VER) || defined(__ICL) || defined(__INTEL_COMPILER)
#include "jstd/config/win32/config.h"
#elif defined(__apple__)
#include "jstd/config/macos/config.h"
#else
#include "jstd/config/default/config.h"
#endif

#include "jstd/config/config_post.h"

#endif // JSTD_CONFIG_CONFIG_H
