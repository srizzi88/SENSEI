//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_openmp_internal_ParallelRadixSortOpenMP_h
#define svtk_m_cont_openmp_internal_ParallelRadixSortOpenMP_h

#include <svtkm/cont/internal/ParallelRadixSortInterface.h>

namespace svtkm
{
namespace cont
{
namespace openmp
{
namespace sort
{
namespace radix
{

SVTKM_DECLARE_RADIX_SORT()
}
}
}
}
} // end namespace svtkm::cont::openmp::sort::radix

#endif // svtk_m_cont_openmp_internal_ParallelRadixSortOpenMP_h
