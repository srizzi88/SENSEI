/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCANormalEstimation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPCANormalEstimation
 * @brief   generate point normals using local tangent planes
 *
 *
 * svtkPCANormalEstimation generates point normals using PCA (principal
 * component analysis).  Basically this estimates a local tangent plane
 * around each sample point p by considering a small neighborhood of points
 * around p, and fitting a plane to the neighborhood (via PCA). A good
 * introductory reference is Hoppe's "Surface reconstruction from
 * unorganized points."
 *
 * To use this filter, specify a neighborhood size. This may have to be set
 * via experimentation. In addition, the user may optionally specify a point
 * locator (instead of the default locator), which is used to accelerate
 * searches around the sample point. Finally, the user should specify how to
 * generate consistently-oriented normals. As computed by PCA, normals may
 * point in arbitrary +/- orientation, which may not be consistent with
 * neighboring normals. There are three methods to address normal
 * consistency: 1) leave the normals as computed, 2) adjust the +/- sign of
 * the normals so that the normals all point towards a specified point, and
 * 3) perform a traversal of the point cloud and flip neighboring normals so
 * that they are mutually consistent.
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
 * svtkPCACurvatureEstimation
 */

#ifndef svtkPCANormalEstimation_h
#define svtkPCANormalEstimation_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkAbstractPointLocator;
class svtkIdList;

class SVTKFILTERSPOINTS_EXPORT svtkPCANormalEstimation : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkPCANormalEstimation* New();
  svtkTypeMacro(svtkPCANormalEstimation, svtkPolyDataAlgorithm);
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

  /**
   * This enum is used to control how normals oriented is controlled.
   */
  enum Style
  {
    AS_COMPUTED = 0,
    POINT = 1,
    GRAPH_TRAVERSAL = 3
  };

  //@{
  /**
   * Configure how the filter addresses consistency in normal
   * oreientation. When initially computed using PCA, a point normal may
   * point in the + or - direction, which may not be consistent with
   * neighboring points. To address this, various strategies have been used
   * to create consistent normals. The simplest approach is to do nothing
   * (AsComputed). Another simple approach is to flip the normal based on its
   * direction with respect to a specified point (i.e., point normals will
   * point towrads the specified point). Finally, a full traversal of points
   * across the graph of neighboring, connected points produces the best
   * results but is computationally expensive.
   */
  svtkSetMacro(NormalOrientation, int);
  svtkGetMacro(NormalOrientation, int);
  void SetNormalOrientationToAsComputed() { this->SetNormalOrientation(AS_COMPUTED); }
  void SetNormalOrientationToPoint() { this->SetNormalOrientation(POINT); }
  void SetNormalOrientationToGraphTraversal() { this->SetNormalOrientation(GRAPH_TRAVERSAL); }
  //@}

  //@{
  /**
   * If the normal orientation is to be consistent with a specified
   * direction, then an orientation point should be set. The sign of the
   * normals will be modified so that they point towards this point. By
   * default, the specified orientation point is (0,0,0).
   */
  svtkSetVector3Macro(OrientationPoint, double);
  svtkGetVectorMacro(OrientationPoint, double, 3);
  //@}

  //@{
  /**
   * The normal orientation can be flipped by enabling this flag.
   */
  svtkSetMacro(FlipNormals, bool);
  svtkGetMacro(FlipNormals, bool);
  svtkBooleanMacro(FlipNormals, bool);
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
  svtkPCANormalEstimation();
  ~svtkPCANormalEstimation() override;

  // IVars
  int SampleSize;
  svtkAbstractPointLocator* Locator;
  int NormalOrientation;
  double OrientationPoint[3];
  bool FlipNormals;

  // Methods used to produce consistent normal orientations
  void TraverseAndFlip(
    svtkPoints* inPts, float* normals, char* pointMap, svtkIdList* wave, svtkIdList* wave2);

  // Pipeline management
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPCANormalEstimation(const svtkPCANormalEstimation&) = delete;
  void operator=(const svtkPCANormalEstimation&) = delete;
};

#endif
