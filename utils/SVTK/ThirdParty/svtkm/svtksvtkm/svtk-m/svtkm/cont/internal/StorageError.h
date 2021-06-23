//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_cont_internal_StorageError_h
#define svtkm_cont_internal_StorageError_h

#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

/// This is an invalid Storage. The point of this class is to include the
/// header file to make this invalid class the default Storage. From that
/// point, you have to specify an appropriate Storage or else get a compile
/// error.
///
struct SVTKM_ALWAYS_EXPORT StorageTagError
{
  // Not implemented.
};
}
}
} // namespace svtkm::cont::internal

#endif //svtkm_cont_internal_StorageError_h
