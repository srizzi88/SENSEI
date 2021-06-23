/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStatisticalOutlierRemoval.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStatisticalOutlierRemoval
 * @brief   remove sparse outlier points
 *
 *
 * The svtkStatisticalOutlierRemoval filter removes sparse outlier points
 * through statistical analysis. The average (mean) distance between points
 * in the point cloud is computed (taking a local sample size around each
 * point); followed by computation of the global standard deviation of
 * distances between points. This global, statistical information is compared
 * against the mean separation distance for each point; those points whose
 * average separation is greater than the user-specified variation in
 * a multiple of standard deviation are removed.
 *
 * Note that while any svtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
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
 * svtkPointCloudFilter svtkRadiusOutlierRemoval svtkExtractPoints
 * svtkThresholdPoints
 */

#ifndef svtkStatisticalOutlierRemoval_h
#define svtkStatisticalOutlierRemoval_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointCloudFilter.h"

class svtkAbstractPointLocator;
class svtkPointSet;

class SVTKFILTERSPOINTS_EXPORT svtkStatisticalOutlierRemoval : public svtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkStatisticalOutlierRemoval* New();
  svtkTypeMacro(svtkStatisticalOutlierRemoval, svtkPointCloudFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * For each point sampled, specify the number of the closest, surrounding
   * points used to compute statistics. By default 25 points are used. Smaller
   * numbers may speed performance.
   */
  svtkSetClampMacro(SampleSize, int, 1, SVTK_INT_MAX);
  svtkGetMacro(SampleSize, int);
  //@}

  //@{
  /**
   * The filter uses this specified standard deviation factor to extract
   * points. By default, points within 1.0 standard deviations (i.e., a
   * StandardDeviationFactor=1.0) of the mean distance to neighboring
   * points are retained.
   */
  svtkSetClampMacro(StandardDeviationFactor, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(StandardDeviationFactor, double);
  //@}

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * surroinding a sample point.
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

  //@{
  /**
   * After execution, return the value of the computed mean. Before execution
   * the value returned is invalid.
   */
  svtkSetClampMacro(ComputedMean, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(ComputedMean, double);
  //@}

  //@{
  /**
   * After execution, return the value of the computed sigma (standard
   * deviation). Before execution the value returned is invalid.
   */
  svtkSetClampMacro(ComputedStandardDeviation, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(ComputedStandardDeviation, double);
  //@}

protected:
  svtkStatisticalOutlierRemoval();
  ~svtkStatisticalOutlierRemoval() override;

  int SampleSize;
  double StandardDeviationFactor;
  svtkAbstractPointLocator* Locator;

  // Derived quantities
  double ComputedMean;
  double ComputedStandardDeviation;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(svtkPointSet* input) override;

private:
  svtkStatisticalOutlierRemoval(const svtkStatisticalOutlierRemoval&) = delete;
  void operator=(const svtkStatisticalOutlierRemoval&) = delete;
};

#endif
