//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_Portals_h
#define svtk_m_worklet_colorconversion_Portals_h

#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

struct MagnitudePortal
{
  template <typename T, int N>
  SVTKM_EXEC auto operator()(const svtkm::Vec<T, N>& values) const
    -> decltype(svtkm::Magnitude(values))
  { //Should we be using RMag?
    return svtkm::Magnitude(values);
  }
};

struct ComponentPortal
{
  svtkm::IdComponent Component;

  ComponentPortal()
    : Component(0)
  {
  }

  ComponentPortal(svtkm::IdComponent comp)
    : Component(comp)
  {
  }

  template <typename T>
  SVTKM_EXEC auto operator()(T&& value) const ->
    typename std::remove_reference<decltype(value[svtkm::IdComponent{}])>::type
  {
    return value[this->Component];
  }
};
}
}
}
#endif
