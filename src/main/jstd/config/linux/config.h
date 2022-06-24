
#ifndef JSTD_CONFIG_LINUX_CONFIG_H
#define JSTD_CONFIG_LINUX_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(__linux__) || defined(__unix__)

#define JSTD_USE_DEFAULT_CONFIG     0

#endif /* __linux__ || __unix__ */

#endif // JSTD_CONFIG_LINUX_CONFIG_H
