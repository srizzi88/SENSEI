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
#ifndef svtkmlib_ImplicitFunctionConverter_h
#define svtkmlib_ImplicitFunctionConverter_h

#include "svtkAcceleratorsSVTKmModule.h"
#include "svtkType.h"    // For svtkMTimeType
#include "svtkmConfig.h" //required for general svtkm setup

#include "svtkm/cont/ImplicitFunctionHandle.h"

class svtkImplicitFunction;

namespace tosvtkm
{

class SVTKACCELERATORSSVTKM_EXPORT ImplicitFunctionConverter
{
public:
  ImplicitFunctionConverter();

  void Set(svtkImplicitFunction*);
  const svtkm::cont::ImplicitFunctionHandle& Get() const;

private:
  svtkImplicitFunction* InFunction;
  svtkm::cont::ImplicitFunctionHandle OutFunction;
  mutable svtkMTimeType MTime;
};

}

#endif // svtkmlib_ImplicitFunctionConverter_h
