
#ifndef JSTD_CONFIG_MACOS_CONFIG_H
#define JSTD_CONFIG_MACOS_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(__apple__) || defined(__APPLE__)

#define JSTD_USE_DEFAULT_CONFIG     0

#endif /* __apple__ || __APPLE__ */

#endif // JSTD_CONFIG_MACOS_CONFIG_H
