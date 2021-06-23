//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#define svtkmlib_Portals_cxx
#include "Portals.h"

#include <svtkm/cont/internal/ArrayPortalFromIterators.h>

namespace tosvtkm
{
// T extern template instantiations
template class SVTKACCELERATORSSVTKM_EXPORT svtkPointsPortal<svtkm::Vec<svtkm::Float32, 3> const>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkPointsPortal<svtkm::Vec<svtkm::Float64, 3> const>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkPointsPortal<svtkm::Vec<svtkm::Float32, 3> >;
template class SVTKACCELERATORSSVTKM_EXPORT svtkPointsPortal<svtkm::Vec<svtkm::Float64, 3> >;
}
