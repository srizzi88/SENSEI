//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_FilterTraits_h
#define svtk_m_filter_FilterTraits_h

#include <svtkm/List.h>

namespace svtkm
{
namespace filter
{

template <typename Derived>
class Filter;


template <typename Filter>
struct FilterTraits
{
  using InputFieldTypeList = typename Filter::SupportedTypes;
  using AdditionalFieldStorage = typename Filter::AdditionalFieldStorage;
};

template <typename DerivedPolicy, typename ListOfTypes>
struct DeduceFilterFieldTypes
{
  using PList = typename DerivedPolicy::FieldTypeList;
  using TypeList = svtkm::ListIntersect<ListOfTypes, PList>;
};
}
}

#endif //svtk_m_filter_FilterTraits_h
