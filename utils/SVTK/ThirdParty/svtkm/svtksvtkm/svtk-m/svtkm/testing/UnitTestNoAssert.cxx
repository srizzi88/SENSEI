//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define SVTKM_NO_ASSERT

#include <svtkm/Assert.h>

int UnitTestNoAssert(int, char* [])
{
  SVTKM_ASSERT(false);
  return 0;
}
