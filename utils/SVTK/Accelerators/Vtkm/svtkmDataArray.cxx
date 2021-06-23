//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 Sandia Corporation.
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#define svtkmDataArray_cxx

#include "svtkmDataArray.h"

template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<char>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<double>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<float>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<int>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<long>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<long long>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<short>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<signed char>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<unsigned char>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<unsigned int>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<unsigned long>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<unsigned long long>;
template class SVTKACCELERATORSSVTKM_EXPORT svtkmDataArray<unsigned short>;
