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
 * @class   svtkmAverageToPoints
 * @brief   Accelerated cell to point interpolation filter.
 *
 * svtkmAverageToPoints is a filter that transforms cell data (i.e., data
 * specified per cell) into point data (i.e., data specified at cell
 * points). The method of transformation is based on averaging the data
 * values of all cells using a particular point. This filter will also
 * pass through any existing point and cell arrays.
 *
 */
#ifndef svtkmAverageToPoints_h
#define svtkmAverageToPoints_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkDataSetAlgorithm.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmAverageToPoints : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkmAverageToPoints, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmAverageToPoints* New();

protected:
  svtkmAverageToPoints();
  ~svtkmAverageToPoints() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmAverageToPoints(const svtkmAverageToPoints&) = delete;
  void operator=(const svtkmAverageToPoints&) = delete;
};

#endif // svtkmAverageToPoints_h
// SVTK-HeaderTest-Exclude: svtkmAverageToPoints.h
