/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphWeightEuclideanDistanceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphWeightEuclideanDistanceFilter
 * @brief   Weights the edges of a
 * graph based on the Euclidean distance between the points.
 *
 *
 * Weights the edges of a graph based on the Euclidean distance between the points.
 */

#ifndef svtkGraphWeightEuclideanDistanceFilter_h
#define svtkGraphWeightEuclideanDistanceFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkGraphWeightFilter.h"

class svtkGraph;

class SVTKFILTERSGENERAL_EXPORT svtkGraphWeightEuclideanDistanceFilter : public svtkGraphWeightFilter
{
public:
  static svtkGraphWeightEuclideanDistanceFilter* New();
  svtkTypeMacro(svtkGraphWeightEuclideanDistanceFilter, svtkGraphWeightFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkGraphWeightEuclideanDistanceFilter() {}
  ~svtkGraphWeightEuclideanDistanceFilter() override {}

  /**
   * Compute the Euclidean distance between the Points defined for the
   * vertices of a specified 'edge'.
   */
  float ComputeWeight(svtkGraph* const graph, const svtkEdgeType& edge) const override;

  /**
   * Ensure that 'graph' has Points defined.
   */
  bool CheckRequirements(svtkGraph* const graph) const override;

private:
  svtkGraphWeightEuclideanDistanceFilter(const svtkGraphWeightEuclideanDistanceFilter&) = delete;
  void operator=(const svtkGraphWeightEuclideanDistanceFilter&) = delete;
};

#endif
