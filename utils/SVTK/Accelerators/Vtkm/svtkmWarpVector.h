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
 * @class   svtkmWarpVector
 * @brief   deform geometry with vector data
 *
 * svtkWarpVector is a filter that modifies point coordinates by moving
 * points along vector times the scale factor. Useful for showing flow
 * profiles or mechanical deformation.
 *
 * The filter passes both its point data and cell data to its output.
 */

#ifndef svtkmWarpVector_h
#define svtkmWarpVector_h

#include "svtkAcceleratorsSVTKmModule.h" // required for correct export
#include "svtkWarpVector.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmWarpVector : public svtkWarpVector
{
public:
  svtkTypeMacro(svtkmWarpVector, svtkWarpVector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmWarpVector* New();

protected:
  svtkmWarpVector();
  ~svtkmWarpVector() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmWarpVector(const svtkmWarpVector&) = delete;
  void operator=(const svtkmWarpVector&) = delete;
};

#endif // svtkmWarpVector_h

// SVTK-HeaderTest-Exclude: svtkmWarpVector.h
