
#ifndef JSTD_BASIC_VLD_DEF_H
#define JSTD_BASIC_VLD_DEF_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

//
// The switch of Visual Leak Detector(vld).
// And this file will not be updated or uploaded to the git remote repository.
// You can edit the value of 'JSTD_ENABLE_VLD' and enable the Visual Leak Detector(vld).
//

//
// Enable os disable the Visual Leak Detector (For Visual Studio Only).
// Note: 0 is disable VLD and 1 is enable VLD, default value is 0.
//
#define JSTD_ENABLE_VLD     1

#endif // JSTD_BASIC_VLD_DEF_H
