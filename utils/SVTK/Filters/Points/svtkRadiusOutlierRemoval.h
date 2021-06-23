/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRadiusOutlierRemoval.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRadiusOutlierRemoval
 * @brief   remove isolated points
 *
 *
 * svtkRadiusOutlierRemoval removes isolated points; i.e., those points that
 * have few neighbors within a specified radius. The user must specify the
 * radius defining the local region, as well as the isolation threshold
 * (i.e., number of neighboring points required for the point to be
 * considered isolated). Optionally, users can specify a point locator to
 * accelerate local neighborhood search operations. (By default a
 * svtkStaticPointLocator will be created.)
 *
 * Note that while any svtkPointSet type can be provided as input, the output
 * is represented by an explicit representation of points via a
 * svtkPolyData. This output polydata will populate its instance of svtkPoints,
 * but no cells will be defined (i.e., no svtkVertex or svtkPolyVertex are
 * contained in the output). Also, after filter execution, the user can
 * request a svtkIdType* map which indicates how the input points were mapped
 * to the output. A value of map[i] (where i is the ith input point) less
 * than 0 means that the ith input point was removed. (See also the
 * superclass documentation for accessing the removed points through the
 * filter's second output.)
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkPointCloudFilter svtkStatisticalOutlierRemoval svtkExtractPoints
 * svtkThresholdPoints svtkImplicitFunction
 */

#ifndef svtkRadiusOutlierRemoval_h
#define svtkRadiusOutlierRemoval_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointCloudFilter.h"

class svtkAbstractPointLocator;
class svtkPointSet;

class SVTKFILTERSPOINTS_EXPORT svtkRadiusOutlierRemoval : public svtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkRadiusOutlierRemoval* New();
  svtkTypeMacro(svtkRadiusOutlierRemoval, svtkPointCloudFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the local search radius.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Specify the number of neighbors that a point must have, within
   * the specified radius, for the point to not be considered isolated.
   */
  svtkSetClampMacro(NumberOfNeighbors, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfNeighbors, int);
  //@}

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient searches to locate near a
   * specified interpolation position.
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

protected:
  svtkRadiusOutlierRemoval();
  ~svtkRadiusOutlierRemoval() override;

  double Radius;
  int NumberOfNeighbors;
  svtkAbstractPointLocator* Locator;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(svtkPointSet* input) override;

private:
  svtkRadiusOutlierRemoval(const svtkRadiusOutlierRemoval&) = delete;
  void operator=(const svtkRadiusOutlierRemoval&) = delete;
};

#endif
