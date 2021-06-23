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
 * @class   svtkmWarpScalar
 * @brief   deform geometry with scalar data
 *
 * svtkmWarpScalar is a filter that modifies point coordinates by moving points
 * along point normals by the scalar amount times the scalar factor with svtkm
 * as its backend.
 * Useful for creating carpet or x-y-z plots.
 *
 * If normals are not present in data, the Normal instance variable will
 * be used as the direction along which to warp the geometry. If normals are
 * present but you would like to use the Normal instance variable, set the
 * UseNormal boolean to true.
 *
 * If XYPlane boolean is set true, then the z-value is considered to be
 * a scalar value (still scaled by scale factor), and the displacement is
 * along the z-axis. If scalars are also present, these are copied through
 * and can be used to color the surface.
 *
 * Note that the filter passes both its point data and cell data to
 * its output, except for normals, since these are distorted by the
 * warping.
 */

#ifndef svtkmWarpScalar_h
#define svtkmWarpScalar_h

#include "svtkAcceleratorsSVTKmModule.h" // required for correct export
#include "svtkWarpScalar.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmWarpScalar : public svtkWarpScalar
{
public:
  svtkTypeMacro(svtkmWarpScalar, svtkWarpScalar);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmWarpScalar* New();

protected:
  svtkmWarpScalar();
  ~svtkmWarpScalar() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmWarpScalar(const svtkmWarpScalar&) = delete;
  void operator=(const svtkmWarpScalar&) = delete;
};

#endif // svtkmWarpScalar_h

// SVTK-HeaderTest-Exclude: svtkmWarpScalar.h
