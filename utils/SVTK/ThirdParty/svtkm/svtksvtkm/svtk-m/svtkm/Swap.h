//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_Swap_h
#define svtk_m_Swap_h

#include <svtkm/internal/ExportMacros.h>

#ifdef __CUDACC__
#include <thrust/swap.h>
#else
#include <algorithm>
#endif

namespace svtkm
{

/// Performs a swap operation. Safe to call from cuda code.
#ifdef __CUDACC__
template <typename T>
SVTKM_EXEC_CONT void Swap(T& a, T& b)
{
  using namespace thrust;
  swap(a, b);
}
#else
template <typename T>
SVTKM_EXEC_CONT void Swap(T& a, T& b)
{
  using namespace std;
  swap(a, b);
}
#endif

} // end namespace svtkm

#endif //svtk_m_Swap_h
