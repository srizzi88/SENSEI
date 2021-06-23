/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointInterpolator2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointInterpolator2D
 * @brief   interpolate point cloud attribute data
 * onto x-y plane using various kernels
 *
 *
 * svtkPointInterpolator2D probes a point cloud Pc (the filter Source) with a
 * set of points P (the filter Input), interpolating the data values from Pc
 * onto P. Note however that the descriptive phrase "point cloud" is a
 * misnomer: Pc can be represented by any svtkDataSet type, with the points of
 * the dataset forming Pc. Similarly, the output P can also be represented by
 * any svtkDataSet type; and the topology/geometry structure of P is passed
 * through to the output along with the newly interpolated arrays. However,
 * this filter presumes that P lies on a plane z=0.0, thus z-coordinates
 * are set accordingly during the interpolation process.
 *
 * The optional boolean flag InterpolateZ is provided for convenience. In
 * effect it turns the source z coordinates into an additional array that is
 * interpolated onto the output data. For example, if the source is a x-y-z
 * LIDAR point cloud, then z can be interpolated onto the output dataset as a
 * vertical elevation(z-coordinate).
 *
 * A key input to this filter is the specification of the interpolation
 * kernel, and the parameters which control the associated interpolation
 * process. Interpolation kernels include Voronoi, Gaussian, Shepard, and SPH
 * (smoothed particle hydrodynamics), with additional kernels to be added in
 * the future. See svtkPointInterpolator for more information.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * For widely spaced points in Pc, or when p is located outside the bounding
 * region of Pc, the interpolation may behave badly and the interpolation
 * process will adapt as necessary to produce output. For example, if the N
 * closest points within R are requested to interpolate p, if N=0 then the
 * interpolation will switch to a different strategy (which can be controlled
 * as in the NullPointsStrategy).
 *
 * @sa
 * svtkPointInterpolator
 */

#ifndef svtkPointInterpolator2D_h
#define svtkPointInterpolator2D_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointInterpolator.h"
#include "svtkStdString.h" // For svtkStdString ivars

class SVTKFILTERSPOINTS_EXPORT svtkPointInterpolator2D : public svtkPointInterpolator
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing.
   */
  static svtkPointInterpolator2D* New();
  svtkTypeMacro(svtkPointInterpolator2D, svtkPointInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify whether to take the z-coordinate values of the source points as
   * attributes to be interpolated. This is in addition to any other point
   * attribute data associated with the source. By default this is enabled.
   */
  svtkSetMacro(InterpolateZ, bool);
  svtkGetMacro(InterpolateZ, bool);
  svtkBooleanMacro(InterpolateZ, bool);
  //@}

  //@{
  /**
   * Specify the name of the output array containing z values. This method is
   * only applicable when InterpolateZ is enabled. By default the output
   * array name is "Elevation".
   */
  svtkSetMacro(ZArrayName, svtkStdString);
  svtkGetMacro(ZArrayName, svtkStdString);
  //@}

protected:
  svtkPointInterpolator2D();
  ~svtkPointInterpolator2D() override;

  // Interpolate z values?
  bool InterpolateZ;

  // Name of output array
  svtkStdString ZArrayName;

  // The driver of the algorithm
  void Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output) override;

private:
  svtkPointInterpolator2D(const svtkPointInterpolator2D&) = delete;
  void operator=(const svtkPointInterpolator2D&) = delete;
};

#endif
