
#ifndef JSTD_BASIC_VLD_H
#define JSTD_BASIC_VLD_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/**********************************************************
 *
 *  Use Visual Leak Detector(vld) for Visual C++,
 *  Homepage: http://vld.codeplex.com/
 *
 **********************************************************/
#ifdef _MSC_VER
#if defined(_DEBUG) || !defined(NDEBUG)

#if defined(JSTD_ENABLE_VLD) && (JSTD_ENABLE_VLD != 0)
// If you have not installed VLD (visual leak detector), please comment out this statement.
#include <vld.h>
#endif //JSTD_ENABLE_VLD

#endif // _DEBUG
#endif // _MSC_VER

#endif // JSTD_BASIC_VLD_H
