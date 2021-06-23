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
 * @class   svtkmThreshold
 * @brief   extracts cells where scalar value in cell satisfies threshold criterion
 *
 * svtkmThreshold is a filter that extracts cells from any dataset type that
 * satisfy a threshold criterion. A cell satisfies the criterion if the
 * scalar value of every point or cell satisfies the criterion. The
 * criterion takes the form of between two values. The output of this
 * filter is an unstructured grid.
 *
 * Note that scalar values are available from the point and cell attribute
 * data. By default, point data is used to obtain scalars, but you can
 * control this behavior. See the AttributeMode ivar below.
 *
 */
#ifndef svtkmThreshold_h
#define svtkmThreshold_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkThreshold.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmThreshold : public svtkThreshold
{
public:
  svtkTypeMacro(svtkmThreshold, svtkThreshold);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmThreshold* New();

protected:
  svtkmThreshold();
  ~svtkmThreshold() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmThreshold(const svtkmThreshold&) = delete;
  void operator=(const svtkmThreshold&) = delete;
};

#endif // svtkmThreshold_h
// SVTK-HeaderTest-Exclude: svtkmThreshold.h
