//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CellFilter_h
#define svtk_m_filter_CellFilter_h

#include <svtkm/filter/FilterField.h>

namespace svtkm
{
namespace filter
{

template <class Derived>
using FilterCell = svtkm::filter::FilterField<Derived>;
}
}
#endif // svtk_m_filter_CellFilter_h
