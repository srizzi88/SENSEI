/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointCloudFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointCloudFilter
 * @brief   abstract class for filtering a point cloud
 *
 *
 * svtkPointCloudFilter serves as a base for classes that filter point clouds.
 * It takes as input any svtkPointSet (which represents points explicitly
 * using svtkPoints) and produces as output an explicit representation of
 * filtered points via a svtkPolyData. This output svtkPolyData will populate
 * its instance of svtkPoints, and typically no cells will be defined (i.e.,
 * no svtkVertex or svtkPolyVertex are contained in the output unless
 * explicitly requested). Also, after filter execution, the user can request
 * a svtkIdType* point map which indicates how the input points were mapped to
 * the output. A value of PointMap[i] < 0 (where i is the ith input point)
 * means that the ith input point was removed. Otherwise PointMap[i]
 * indicates the position in the output svtkPoints array (point cloud).
 *
 * Optionally the filter may produce a second output. This second output is
 * another svtkPolyData with a svtkPoints that contains the points that were
 * removed during processing. To produce this second output, you must enable
 * GenerateOutliers. If this optional, second output is created, then the
 * contents of the PointMap are modified as well. In this case, a PointMap[i]
 * < 0 means that the ith input point has been mapped to the (-PointMap[i])-1
 * position in the second output's svtkPoints.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * The filter copies point attributes from input to output consistent
 * with the filtering operation.
 *
 * @warning
 * It is convenient to use svtkPointGaussianMapper to render the points (since
 * this mapper does not require cells to be defined, and it is quite fast).
 *
 * @sa
 * svtkRadiusOutlierRemoval svtkPointGaussianMapper svtkThresholdPoints
 */

#ifndef svtkPointCloudFilter_h
#define svtkPointCloudFilter_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPointSet;
class svtkPolyData;

class SVTKFILTERSPOINTS_EXPORT svtkPointCloudFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to obtain type information, and print information.
   */
  svtkTypeMacro(svtkPointCloudFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Retrieve a map which indicates, on a point-by-point basis, where each
   * input point was placed into the output. In other words, map[i] indicates
   * where the ith input point is located in the output array of points. If
   * map[i] < 0, then the ith input point was removed during filter
   * execution.  This method returns valid information only after the filter
   * executes.
   */
  const svtkIdType* GetPointMap();

  /**
   * Return the number of points removed after filter execution. The
   * information returned is valid only after the filter executes.
   */
  svtkIdType GetNumberOfPointsRemoved();

  //@{
  /**
   * If this method is enabled (true), then a second output will be created
   * that contains the outlier points. By default this is off (false).  Note
   * that if enabled, the PointMap is modified as well: the outlier points
   * are listed as well, with similar meaning, except their value is negated
   * and shifted by -1.
   */
  svtkSetMacro(GenerateOutliers, bool);
  svtkGetMacro(GenerateOutliers, bool);
  svtkBooleanMacro(GenerateOutliers, bool);
  //@}

  //@{
  /**
   * If this method is enabled (true), then the outputs will contain a vertex
   * cells (i.e., a svtkPolyVertex for each output). This takes a lot more
   * memory but some SVTK filters need cells to function properly. By default
   * this is off (false).
   */
  svtkSetMacro(GenerateVertices, bool);
  svtkGetMacro(GenerateVertices, bool);
  svtkBooleanMacro(GenerateVertices, bool);
  //@}

protected:
  svtkPointCloudFilter();
  ~svtkPointCloudFilter() override;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned on error.
  virtual int FilterPoints(svtkPointSet* input) = 0;

  // Keep track of which points are removed through the point map
  svtkIdType* PointMap;
  svtkIdType NumberOfPointsRemoved;

  // Does a second output need to be created?
  bool GenerateOutliers;

  // Should output vertex cells be created?
  bool GenerateVertices;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void GenerateVerticesIfRequested(svtkPolyData* output);

private:
  svtkPointCloudFilter(const svtkPointCloudFilter&) = delete;
  void operator=(const svtkPointCloudFilter&) = delete;
};

#endif
