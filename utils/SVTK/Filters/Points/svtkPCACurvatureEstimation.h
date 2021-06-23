/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCACurvatureEstimation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPCACurvatureEstimation
 * @brief   generate curvature estimates using
 * principal component analysis
 *
 *
 * svtkPCACurvatureEstimation generates point normals using PCA (principal
 * component analysis).  Basically this estimates a local tangent plane
 * around sample point p by considering a small neighborhood of points
 * around p, and fitting a plane to the neighborhood (via PCA). A good
 * introductory reference is Hoppe's "Surface reconstruction from
 * unorganized points."
 *
 * To use this filter, specify a neighborhood size. This may have to be set
 * via experimentation. Optionally a point locator can be specified (instead
 * of the default locator), which is used to accelerate searches around a
 * sample point. Finally, the user should specify how to generate
 * consistently-oriented normals. As computed by PCA, normals may point in
 * +/- orientation, which may not be consistent with neighboring normals.
 *
 * The output of this filter is the same as the input except that a normal
 * per point is produced. (Note that these are unit normals.) While any
 * svtkPointSet type can be provided as input, the output is represented by an
 * explicit representation of points via a svtkPolyData. This output polydata
 * will populate its instance of svtkPoints, but no cells will be defined
 * (i.e., no svtkVertex or svtkPolyVertex are contained in the output).
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkPCANormalEstimation
 */

#ifndef svtkPCACurvatureEstimation_h
#define svtkPCACurvatureEstimation_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkAbstractPointLocator;

class SVTKFILTERSPOINTS_EXPORT svtkPCACurvatureEstimation : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkPCACurvatureEstimation* New();
  svtkTypeMacro(svtkPCACurvatureEstimation, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * For each sampled point, specify the number of the closest, surrounding
   * points used to estimate the normal (the so called k-neighborhood). By
   * default 25 points are used. Smaller numbers may speed performance at the
   * cost of accuracy.
   */
  svtkSetClampMacro(SampleSize, int, 1, SVTK_INT_MAX);
  svtkGetMacro(SampleSize, int);
  //@}

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

protected:
  svtkPCACurvatureEstimation();
  ~svtkPCACurvatureEstimation() override;

  // IVars
  int SampleSize;
  svtkAbstractPointLocator* Locator;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPCACurvatureEstimation(const svtkPCACurvatureEstimation&) = delete;
  void operator=(const svtkPCACurvatureEstimation&) = delete;
};

#endif
