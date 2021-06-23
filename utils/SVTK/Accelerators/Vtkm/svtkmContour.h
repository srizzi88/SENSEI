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
 * @class   svtkmContour
 * @brief   generate isosurface(s) from volume
 *
 * svtkmContour is a filter that takes as input a volume (e.g., 3D
 * structured point set) and generates on output one or more isosurfaces.
 * One or more contour values must be specified to generate the isosurfaces.
 * Alternatively, you can specify a min/max scalar range and the number of
 * contours to generate a series of evenly spaced contour values.
 *
 * @warning
 * This filter is currently only supports 3D volumes. If you are interested in
 * contouring other types of data, use the general svtkContourFilter. If you
 * want to contour an image (i.e., a volume slice), use svtkMarchingSquares.
 *
 */

#ifndef svtkmContour_h
#define svtkmContour_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkContourFilter.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmContour : public svtkContourFilter
{
public:
  svtkTypeMacro(svtkmContour, svtkContourFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmContour* New();

protected:
  svtkmContour();
  ~svtkmContour() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmContour(const svtkmContour&) = delete;
  void operator=(const svtkmContour&) = delete;
};

#endif // svtkmContour_h
// SVTK-HeaderTest-Exclude: svtkmContour.h
