/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDensifyPointCloudFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDensifyPointCloudFilter
 * @brief   add points to a point cloud to make it denser
 *
 * svtkDensifyPointCloudFilter adds new points to an input point cloud. The
 * new points are created in such a way that all points in any local
 * neighborhood are within a target distance of one another. Optionally,
 * attribute data can be interpolated from the input point cloud as well.
 *
 * A high-level overview of the algorithm is as follows. For each input
 * point, the distance to all points in its neighborhood is computed. If any
 * of its neighbors is further than the target distance, the edge connecting
 * the point and its neighbor is bisected and a new point is inserted at the
 * bisection point (optionally the attribute data is interpolated as well). A
 * single pass is completed once all the input points are visited. Then the
 * process repeats to the limit of the maximum number of iterations.
 *
 * @warning
 * This class can generate a lot of points very quickly. The maximum number
 * of iterations is by default set to =1.0 for this reason. Increase the
 * number of iterations very carefully. Also the MaximumNumberOfPoints
 * data member can be set to limit the explosion of points. It is also
 * recommended that a N closest neighborhood is used.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkVoxelGridFilter svtkEuclideanClusterExtraction svtkBoundedPointSource
 */

#ifndef svtkDensifyPointCloudFilter_h
#define svtkDensifyPointCloudFilter_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSPOINTS_EXPORT svtkDensifyPointCloudFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkDensifyPointCloudFilter* New();
  svtkTypeMacro(svtkDensifyPointCloudFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This enum is used to specify how the local point neighborhood is
   * defined.  A radius-based neighborhood in one where all points inside a
   * specified radius is part of the neighborhood. A N closest neighborhood is
   * one in which the N closest points are part of the neighborhood. (Note that
   * in some cases, if points are precisely the same distance apart, the N closest
   * may not return all points within an expected radius.)
   */
  enum NeighborhoodType
  {
    RADIUS = 0,
    N_CLOSEST = 1
  };

  //@{
  /**
   * Specify how the local point neighborhood is defined. By default an N
   * closest neighborhood is used. This tends to avoid explosive point
   * creation.
   */
  svtkSetMacro(NeighborhoodType, int);
  svtkGetMacro(NeighborhoodType, int);
  void SetNeighborhoodTypeToRadius() { this->SetNeighborhoodType(RADIUS); }
  void SetNeighborhoodTypeToNClosest() { this->SetNeighborhoodType(N_CLOSEST); }
  //@}

  //@{
  /**
   * Define a local neighborhood for each point in terms of a local
   * radius. By default, the radius is 1.0. This data member is relevant only
   * if the neighborhood type is RADIUS.
   */
  svtkSetClampMacro(Radius, double, 1, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Define a local neighborhood in terms of the N closest points. By default
   * the number of the closest points is =6. This data member is relevant
   * only if the neighborhood type is N_CLOSEST.
   */
  svtkSetClampMacro(NumberOfClosestPoints, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfClosestPoints, int);
  //@}

  //@{
  /**
   * Set / get the target point distance. Points will be created in an
   * iterative fashion until all points in their local neighborhood are the
   * target distance apart or less. Note that the process may terminate early
   * due to the limit on the maximum number of iterations. By default the target
   * distance is set to 0.5. Note that the TargetDistance should be less than
   * the Radius or nothing will change on output.
   */
  svtkSetClampMacro(TargetDistance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(TargetDistance, double);
  //@}

  //@{
  /**
   * The maximum number of iterations to run. By default the maximum is
   * one.
   */
  svtkSetClampMacro(MaximumNumberOfIterations, int, 1, SVTK_SHORT_MAX);
  svtkGetMacro(MaximumNumberOfIterations, int);
  //@}

  //@{
  /**
   * Set a limit on the maximum number of points that can be created. This
   * data member serves as a crude barrier to explosive point creation; it does
   * not guarantee that precisely these many points will be created. Once this
   * limit is hit, it may result in premature termination of the
   * algorithm. Consider it a pressure relief valve.
   */
  svtkSetClampMacro(MaximumNumberOfPoints, svtkIdType, 1, SVTK_ID_MAX);
  svtkGetMacro(MaximumNumberOfPoints, svtkIdType);
  //@}

  //@{
  /**
   * Turn on/off the interpolation of attribute data from the input point
   * cloud to new, added points.
   */
  svtkSetMacro(InterpolateAttributeData, bool);
  svtkGetMacro(InterpolateAttributeData, bool);
  svtkBooleanMacro(InterpolateAttributeData, bool);
  //@}

protected:
  svtkDensifyPointCloudFilter();
  ~svtkDensifyPointCloudFilter() override;

  // Data members
  int NeighborhoodType;
  double Radius;
  int NumberOfClosestPoints;
  double TargetDistance;
  int MaximumNumberOfIterations;
  bool InterpolateAttributeData;
  svtkIdType MaximumNumberOfPoints;

  // Pipeline management
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDensifyPointCloudFilter(const svtkDensifyPointCloudFilter&) = delete;
  void operator=(const svtkDensifyPointCloudFilter&) = delete;
};

#endif
