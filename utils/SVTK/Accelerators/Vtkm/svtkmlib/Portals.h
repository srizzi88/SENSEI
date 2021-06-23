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

#ifndef svtkmlib_Portals_h
#define svtkmlib_Portals_h

#include "PortalTraits.h"
#include "svtkAcceleratorsSVTKmModule.h"
#include "svtkmConfig.h" //required for general svtkm setup

class svtkDataArray;
class svtkPoints;

#include <svtkm/cont/internal/IteratorFromArrayPortal.h>

namespace tosvtkm
{

template <typename Type, typename SVTKDataArrayType_>
class SVTKM_ALWAYS_EXPORT svtkArrayPortal
{
  static const int NUM_COMPONENTS = svtkm::VecTraits<Type>::NUM_COMPONENTS;

public:
  typedef SVTKDataArrayType_ SVTKDataArrayType;
  using ValueType = typename svtkPortalTraits<Type>::Type;
  using ComponentType = typename svtkPortalTraits<Type>::ComponentType;

  SVTKM_EXEC_CONT
  svtkArrayPortal();

  SVTKM_CONT
  svtkArrayPortal(SVTKDataArrayType* array, svtkm::Id size);

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->Size; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  inline ValueType Get(svtkm::Id index) const;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  inline void Set(svtkm::Id index, const ValueType& value) const;

  typedef svtkm::cont::internal::IteratorFromArrayPortal<svtkArrayPortal> IteratorType;

  SVTKM_CONT
  IteratorType GetIteratorBegin() const { return IteratorType(*this, 0); }

  SVTKM_CONT
  IteratorType GetIteratorEnd() const { return IteratorType(*this, this->Size); }

  SVTKM_CONT
  SVTKDataArrayType* GetVtkData() const { return this->SVTKData; }

private:
  SVTKDataArrayType* SVTKData;
  svtkm::Id Size;
};

template <typename Type>
class SVTKM_ALWAYS_EXPORT svtkPointsPortal
{
  static const int NUM_COMPONENTS = svtkm::VecTraits<Type>::NUM_COMPONENTS;

public:
  using ValueType = typename svtkPortalTraits<Type>::Type;
  using ComponentType = typename svtkPortalTraits<Type>::ComponentType;

  SVTKM_EXEC_CONT
  svtkPointsPortal();

  SVTKM_CONT
  svtkPointsPortal(svtkPoints* points, svtkm::Id size);

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->Size; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  inline ValueType Get(svtkm::Id index) const;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  inline void Set(svtkm::Id index, const ValueType& value) const;

  typedef svtkm::cont::internal::IteratorFromArrayPortal<svtkPointsPortal> IteratorType;

  SVTKM_CONT
  IteratorType GetIteratorBegin() const { return IteratorType(*this, 0); }

  SVTKM_CONT
  IteratorType GetIteratorEnd() const { return IteratorType(*this, this->Size); }

  SVTKM_CONT
  svtkPoints* GetVtkData() const { return Points; }

private:
  svtkPoints* Points;
  ComponentType* Array;
  svtkm::Id Size;
};
}

#ifndef svtkmlib_Portals_cxx
#include <svtkm/cont/internal/ArrayPortalFromIterators.h>
namespace tosvtkm
{
// T extern template instantiations
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT
  svtkPointsPortal<svtkm::Vec<svtkm::Float32, 3> const>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT
  svtkPointsPortal<svtkm::Vec<svtkm::Float64, 3> const>;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT
  svtkPointsPortal<svtkm::Vec<svtkm::Float32, 3> >;
extern template class SVTKACCELERATORSSVTKM_TEMPLATE_EXPORT
  svtkPointsPortal<svtkm::Vec<svtkm::Float64, 3> >;
}

#endif // defined svtkmlib_Portals_cxx

#include "Portals.hxx"
#endif // svtkmlib_Portals_h
