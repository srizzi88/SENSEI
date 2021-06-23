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
/**
 * @class svtkmClip
 * @brief Clip a dataset using the accelerated svtk-m Clip filter.
 *
 * Clip a dataset using either a given value or by using an svtkImplicitFunction
 * Currently the supported implicit functions are Box, Plane, and Sphere.
 *
 */

#ifndef svtkmClip_h
#define svtkmClip_h

#include "svtkAcceleratorsSVTKmModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

#include <memory> // For std::unique_ptr

class svtkImplicitFunction;

namespace tosvtkm
{

class ImplicitFunctionConverter;

} // namespace tosvtkm

class SVTKACCELERATORSSVTKM_EXPORT svtkmClip : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkmClip* New();
  svtkTypeMacro(svtkmClip, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The scalar value to use when clipping the dataset. Values greater than
   * ClipValue are preserved in the output dataset. Default is 0.
   */
  svtkGetMacro(ClipValue, double);
  svtkSetMacro(ClipValue, double);

  /**
   * If true, all input point data arrays will be mapped onto the output
   * dataset. Default is true.
   */
  svtkGetMacro(ComputeScalars, bool);
  svtkSetMacro(ComputeScalars, bool);

  /**
   * Set the implicit function with which to perform the clipping. If set,
   * \c ClipValue is ignored and the clipping is performed using the implicit
   * function.
   */
  void SetClipFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ClipFunction, svtkImplicitFunction);

  svtkMTimeType GetMTime() override;

protected:
  svtkmClip();
  ~svtkmClip() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  double ClipValue;
  bool ComputeScalars;

  svtkImplicitFunction* ClipFunction;
  std::unique_ptr<tosvtkm::ImplicitFunctionConverter> ClipFunctionConverter;

private:
  svtkmClip(const svtkmClip&) = delete;
  void operator=(const svtkmClip&) = delete;
};

#endif // svtkmClip_h
// SVTK-HeaderTest-Exclude: svtkmClip.h
