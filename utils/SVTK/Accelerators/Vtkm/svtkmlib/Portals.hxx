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

#ifndef svtkmlib_Portals_hxx
#define svtkmlib_Portals_hxx

#include "svtkDataArray.h"
#include "svtkPoints.h"

#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>
#include <svtkm/cont/internal/IteratorFromArrayPortal.h>
#include <svtkm/internal/Assume.h>

namespace
{

template <int N>
struct fillComponents
{
  template <typename T, typename Tuple>
  SVTKM_EXEC void operator()(T* t, const Tuple& tuple) const
  {
    fillComponents<N - 1>()(t, tuple);
    t[N - 1] = svtkm::VecTraits<Tuple>::GetComponent(tuple, N - 1);
  }
};

template <>
struct fillComponents<1>
{
  template <typename T, typename Tuple>
  SVTKM_EXEC void operator()(T* t, const Tuple& tuple) const
  {
    t[0] = svtkm::VecTraits<Tuple>::GetComponent(tuple, 0);
  }
};
}

namespace tosvtkm
{

//------------------------------------------------------------------------------
template <typename VType, typename SVTKDataArrayType>
SVTKM_EXEC_CONT svtkArrayPortal<VType, SVTKDataArrayType>::svtkArrayPortal()
  : SVTKData(nullptr)
  , Size(0)
{
}

//------------------------------------------------------------------------------
template <typename VType, typename SVTKDataArrayType>
SVTKM_CONT svtkArrayPortal<VType, SVTKDataArrayType>::svtkArrayPortal(
  SVTKDataArrayType* array, svtkm::Id size)
  : SVTKData(array)
  , Size(size)
{
  SVTKM_ASSERT(this->GetNumberOfValues() >= 0);
}

//------------------------------------------------------------------------------
template <typename VType, typename SVTKDataArrayType>
typename svtkArrayPortal<VType, SVTKDataArrayType>::ValueType SVTKM_EXEC
svtkArrayPortal<VType, SVTKDataArrayType>::Get(svtkm::Id index) const
{
  SVTKM_ASSUME(this->SVTKData->GetNumberOfComponents() == NUM_COMPONENTS);

  ValueType tuple;
  for (int j = 0; j < NUM_COMPONENTS; ++j)
  {
    ComponentType temp = this->SVTKData->GetTypedComponent(index, j);
    svtkPortalTraits<ValueType>::SetComponent(tuple, j, temp);
  }
  return tuple;
}

//------------------------------------------------------------------------------
template <typename VType, typename SVTKDataArrayType>
SVTKM_EXEC void svtkArrayPortal<VType, SVTKDataArrayType>::Set(
  svtkm::Id index, const ValueType& value) const
{
  SVTKM_ASSUME((this->SVTKData->GetNumberOfComponents() == NUM_COMPONENTS));

  for (int j = 0; j < NUM_COMPONENTS; ++j)
  {
    ComponentType temp = svtkPortalTraits<ValueType>::GetComponent(value, j);
    this->SVTKData->SetTypedComponent(index, j, temp);
  }
}

//------------------------------------------------------------------------------
template <typename Type>
SVTKM_EXEC_CONT svtkPointsPortal<Type>::svtkPointsPortal()
  : Points(nullptr)
  , Array(nullptr)
  , Size(0)
{
}

//------------------------------------------------------------------------------
template <typename Type>
SVTKM_CONT svtkPointsPortal<Type>::svtkPointsPortal(svtkPoints* points, svtkm::Id size)
  : Points(points)
  , Array(static_cast<ComponentType*>(points->GetVoidPointer(0)))
  , Size(size)
{
  SVTKM_ASSERT(this->GetNumberOfValues() >= 0);
}

//------------------------------------------------------------------------------
template <typename Type>
typename svtkPointsPortal<Type>::ValueType SVTKM_EXEC svtkPointsPortal<Type>::Get(svtkm::Id index) const
{
  const ComponentType* const raw = this->Array + (index * NUM_COMPONENTS);
  return ValueType(raw[0], raw[1], raw[2]);
}

//------------------------------------------------------------------------------
template <typename Type>
SVTKM_EXEC void svtkPointsPortal<Type>::Set(svtkm::Id index, const ValueType& value) const
{
  ComponentType* rawArray = this->Array + (index * NUM_COMPONENTS);
  // use template magic to auto unroll insertion
  fillComponents<NUM_COMPONENTS>()(rawArray, value);
}
}

#endif // svtkmlib_Portals_hxx
