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
 * @brief   Accelerated point to cell interpolation filter.
 *
 * svtkmAverageToPoints is a filter that transforms point data (i.e., data
 * specified at cell points) into cell data (i.e., data specified per cell).
 * The method of transformation is based on averaging the data
 * values of all points used by particular cell. This filter will also
 * pass through any existing point and cell arrays.
 *
 */

#ifndef svtkmAverageToCells_h
#define svtkmAverageToCells_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkDataSetAlgorithm.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmAverageToCells : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkmAverageToCells, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmAverageToCells* New();

protected:
  svtkmAverageToCells();
  ~svtkmAverageToCells() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmAverageToCells(const svtkmAverageToCells&) = delete;
  void operator=(const svtkmAverageToCells&) = delete;
};

#endif // svtkmAverageToCells_h
// SVTK-HeaderTest-Exclude: svtkmAverageToCells.h
