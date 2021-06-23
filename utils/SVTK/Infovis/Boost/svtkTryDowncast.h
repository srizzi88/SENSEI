/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTryDowncast.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkTryDowncast_h
#define svtkTryDowncast_h

#include "svtkDenseArray.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector.hpp>

// These are lists of standard SVTK types.  End-users will have to choose these when they implement
// their algorithms.

// Description:
// Enumerates all integer SVTK types
typedef boost::mpl::vector<svtkTypeUInt8, svtkTypeInt8, svtkTypeUInt16, svtkTypeInt16, svtkTypeUInt32,
  svtkTypeInt32, svtkTypeUInt64, svtkTypeInt64, svtkIdType>
  svtkIntegerTypes;
// Description:
// Enumerates all floating-point SVTK types
typedef boost::mpl::vector<svtkTypeFloat32, svtkTypeFloat64> svtkFloatingPointTypes;
// Description:
// Enumerates all numeric SVTK types
typedef boost::mpl::joint_view<svtkIntegerTypes, svtkFloatingPointTypes> svtkNumericTypes;
// Description:
// Enumerates all string SVTK types
typedef boost::mpl::vector<svtkStdString, svtkUnicodeString> svtkStringTypes;
// Description:
// Enumerates all SVTK types
typedef boost::mpl::joint_view<svtkNumericTypes, svtkStringTypes> svtkAllTypes;

// End-users can ignore these, they're the guts of the beast ...
template <template <typename> class TargetT, typename FunctorT>
class svtkTryDowncastHelper1
{
public:
  svtkTryDowncastHelper1(svtkObject* source1, FunctorT functor, bool& succeeded)
    : Source1(source1)
    , Functor(functor)
    , Succeeded(succeeded)
  {
  }

  template <typename ValueT>
  void operator()(ValueT) const
  {
    if (Succeeded)
      return;

    TargetT<ValueT>* const target1 = TargetT<ValueT>::SafeDownCast(Source1);
    if (target1)
    {
      Succeeded = true;
      this->Functor(target1);
    }
  }

  svtkObject* Source1;
  FunctorT Functor;
  bool& Succeeded;

private:
  svtkTryDowncastHelper1& operator=(const svtkTryDowncastHelper1&);
};

template <template <typename> class TargetT, typename FunctorT>
class svtkTryDowncastHelper2
{
public:
  svtkTryDowncastHelper2(svtkObject* source1, svtkObject* source2, FunctorT functor, bool& succeeded)
    : Source1(source1)
    , Source2(source2)
    , Functor(functor)
    , Succeeded(succeeded)
  {
  }

  template <typename ValueT>
  void operator()(ValueT) const
  {
    if (Succeeded)
      return;

    TargetT<ValueT>* const target1 = TargetT<ValueT>::SafeDownCast(Source1);
    TargetT<ValueT>* const target2 = TargetT<ValueT>::SafeDownCast(Source2);
    if (target1 && target2)
    {
      Succeeded = true;
      this->Functor(target1, target2);
    }
  }

  svtkObject* Source1;
  svtkObject* Source2;
  FunctorT Functor;
  bool& Succeeded;

private:
  svtkTryDowncastHelper2& operator=(const svtkTryDowncastHelper2&);
};

template <template <typename> class TargetT, typename FunctorT>
class svtkTryDowncastHelper3
{
public:
  svtkTryDowncastHelper3(
    svtkObject* source1, svtkObject* source2, svtkObject* source3, FunctorT functor, bool& succeeded)
    : Source1(source1)
    , Source2(source2)
    , Source3(source3)
    , Functor(functor)
    , Succeeded(succeeded)
  {
  }

  template <typename ValueT>
  void operator()(ValueT) const
  {
    if (Succeeded)
      return;

    TargetT<ValueT>* const target1 = TargetT<ValueT>::SafeDownCast(Source1);
    TargetT<ValueT>* const target2 = TargetT<ValueT>::SafeDownCast(Source2);
    TargetT<ValueT>* const target3 = TargetT<ValueT>::SafeDownCast(Source3);
    if (target1 && target2 && target3)
    {
      Succeeded = true;
      this->Functor(target1, target2, target3);
    }
  }

  svtkObject* Source1;
  svtkObject* Source2;
  svtkObject* Source3;
  FunctorT Functor;
  bool& Succeeded;

private:
  svtkTryDowncastHelper3& operator=(const svtkTryDowncastHelper3&);
};

template <template <typename> class TargetT, typename TypesT, typename FunctorT>
bool svtkTryDowncast(svtkObject* source1, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(
    svtkTryDowncastHelper1<TargetT, FunctorT>(source1, functor, succeeded));
  return succeeded;
}

template <template <typename> class TargetT, typename TypesT, typename FunctorT>
bool svtkTryDowncast(svtkObject* source1, svtkObject* source2, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(
    svtkTryDowncastHelper2<TargetT, FunctorT>(source1, source2, functor, succeeded));
  return succeeded;
}

template <template <typename> class TargetT, typename TypesT, typename FunctorT>
bool svtkTryDowncast(svtkObject* source1, svtkObject* source2, svtkObject* source3, FunctorT functor)
{
  bool succeeded = false;
  boost::mpl::for_each<TypesT>(
    svtkTryDowncastHelper3<TargetT, FunctorT>(source1, source2, source3, functor, succeeded));
  return succeeded;
}

#endif
// SVTK-HeaderTest-Exclude: svtkTryDowncast.h
