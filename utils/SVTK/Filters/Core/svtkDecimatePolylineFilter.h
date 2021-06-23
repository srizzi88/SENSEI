/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDecimatePolylineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDecimatePolylineFilter
 * @brief   reduce the number of lines in a polyline
 *
 * svtkDecimatePolylineFilter is a filter to reduce the number of lines in a
 * polyline. The algorithm functions by evaluating an error metric for each
 * vertex (i.e., the distance of the vertex to a line defined from the two
 * vertices on either side of the vertex). Then, these vertices are placed
 * into a priority queue, and those with smaller errors are deleted first.
 * The decimation continues until the target reduction is reached. While the
 * filter will not delete end points, it will decimate closed loops down to a
 * single line, thereby changing topology.
 *
 * Note that a maximum error value (expressed in world coordinates) can also
 * be specified. This may limit the amount of decimation so the target
 * reduction may not be met. By setting the maximum error value to a very
 * small number, colinear points can be eliminated.
 *
 * @warning
 * This algorithm is a very simple implementation that overlooks some
 * potential complexities. For example, if a vertex is multiply connected,
 * meaning that it is used by multiple distinct polylines, then the extra
 * topological constraints are ignored. This can produce less than optimal
 * results.
 *
 * @sa
 * svtkDecimate svtkDecimateProp svtkQuadricClustering svtkQuadricDecimation
 */

#ifndef svtkDecimatePolylineFilter_h
#define svtkDecimatePolylineFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkSmartPointer.h"      // Needed for SP ivars

#include "svtkPolyDataAlgorithm.h"

class svtkPriorityQueue;

class SVTKFILTERSCORE_EXPORT svtkDecimatePolylineFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkDecimatePolylineFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Instantiate this object with a target reduction of 0.90.
   */
  static svtkDecimatePolylineFilter* New();

  //@{
  /**
   * Specify the desired reduction in the total number of polygons (e.g., if
   * TargetReduction is set to 0.9, this filter will try to reduce the data set
   * to 10% of its original size).
   */
  svtkSetClampMacro(TargetReduction, double, 0.0, 1.0);
  svtkGetMacro(TargetReduction, double);
  //@}

  //@{
  /**
   * Set the largest decimation error that is allowed during the decimation
   * process. This may limit the maximum reduction that may be achieved. The
   * maximum error is specified as a fraction of the maximum length of
   * the input data bounding box.
   */
  svtkSetClampMacro(MaximumError, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaximumError, double);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkDecimatePolylineFilter();
  ~svtkDecimatePolylineFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  class Polyline;
  double ComputeError(svtkPolyData* input, Polyline* polyline, svtkIdType id);

  svtkSmartPointer<svtkPriorityQueue> PriorityQueue;
  double TargetReduction;
  double MaximumError;
  int OutputPointsPrecision;

private:
  svtkDecimatePolylineFilter(const svtkDecimatePolylineFilter&) = delete;
  void operator=(const svtkDecimatePolylineFilter&) = delete;
};

#endif
