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
 * @class   svtkmImageConnectivity
 * @brief   Label regions inside an image by connectivity
 *
 * svtkmImageConnectivity will identify connected regions within an
 * image and label them.
 * The filter finds groups of points that have the same field value and are
 * connected together through their topology. Any point is considered to be
 * connected to its Moore neighborhood:
 * - 8 neighboring points for 2D
 * - 27 neighboring points for 3D
 *
 * The active field passed to the filter must be associated with the points.
 * The result of the filter is a point field of type svtkIdType.
 * Each entry in the point field will be a number that identifies to which
 * region it belongs. By default, this output point field is named “component”.
 *
 * @sa
 * svtkConnectivityFilter, svtkImageConnectivityFilter
 */

#ifndef svtkmImageConnectivity_h
#define svtkmImageConnectivity_h

#include "svtkAcceleratorsSVTKmModule.h" //required for correct implementation
#include "svtkImageAlgorithm.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmImageConnectivity : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkmImageConnectivity, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkmImageConnectivity* New();

protected:
  svtkmImageConnectivity();
  ~svtkmImageConnectivity() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmImageConnectivity(const svtkmImageConnectivity&) = delete;
  void operator=(const svtkmImageConnectivity&) = delete;
};

#endif // svtkmImageConnectivity_h
// SVTK-HeaderTest-Exclude: svtkmImageConnectivity.h
