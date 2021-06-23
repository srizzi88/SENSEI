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
 * @class   svtkmCoordinateSystemTransform
 * @brief   transform a coordinate system between Cartesian&Cylindrical and
 *          Cartesian&Spherical
 *
 * svtkmCoordinateSystemTransform is a filter that transforms a coordinate system
 * between Cartesian&Cylindrical and Cartesian&Spherical.
 */

#ifndef svtkmCoordinateSystemTransform_h
#define svtkmCoordinateSystemTransform_h

#include "svtkAcceleratorsSVTKmModule.h" // required for correct export
#include "svtkPointSetAlgorithm.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmCoordinateSystemTransform : public svtkPointSetAlgorithm
{
  enum struct TransformTypes
  {
    None,
    CarToCyl,
    CylToCar,
    CarToSph,
    SphToCar
  };

public:
  svtkTypeMacro(svtkmCoordinateSystemTransform, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmCoordinateSystemTransform* New();

  void SetCartesianToCylindrical();
  void SetCylindricalToCartesian();

  void SetCartesianToSpherical();
  void SetSphericalToCartesian();

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkmCoordinateSystemTransform();
  ~svtkmCoordinateSystemTransform() override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmCoordinateSystemTransform(const svtkmCoordinateSystemTransform&) = delete;
  void operator=(const svtkmCoordinateSystemTransform&) = delete;

  TransformTypes TransformType;
};

#endif // svtkmCoordinateSystemTransform_h

// SVTK-HeaderTest-Exclude: svtkmCoordinateSystemTransform.h
