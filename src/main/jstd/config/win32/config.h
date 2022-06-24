
#ifndef JSTD_CONFIG_WIN32_CONFIG_H
#define JSTD_CONFIG_WIN32_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(_WIN32) || defined(_MSC_VER)

#define JSTD_USE_DEFAULT_CONFIG     0

//
// See: http://msdn.microsoft.com/zh-cn/library/e5ewb1h3%28v=vs.90%29.aspx
// See: http://msdn.microsoft.com/en-us/library/x98tx3cf.aspx
//
#if defined(JSTD_USE_CRTDBG_CHECK) && (JSTD_USE_CRTDBG_CHECK != 0)
#if defined(_DEBUG) || !defined(NDEBUG)
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#endif /* _DEBUG */
#endif /* JSTD_USE_CRTDBG_CHECK */

#endif // _WIN32 || _MSC_VER

#endif // JSTD_CONFIG_WIN32_CONFIG_H
