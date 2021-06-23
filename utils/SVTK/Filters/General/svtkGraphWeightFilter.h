/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphWeightFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphWeightFilter
 * @brief   Base class for filters that weight graph
 * edges.
 *
 *
 * svtkGraphWeightFilter is the abstract base class that provides an interface
 * for classes that apply weights to graph edges. The weights are added
 * as a svtkFloatArray named "Weights."
 * The ComputeWeight function must be implemented to provide the function of two
 * vertices which determines the weight of each edge.
 * The CheckRequirements function can be implemented if you wish to ensure
 * that the input graph has all of the properties that will be required
 * by the ComputeWeight function.
 */

#ifndef svtkGraphWeightFilter_h
#define svtkGraphWeightFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkGraphAlgorithm.h"

class svtkGraph;

class SVTKFILTERSGENERAL_EXPORT svtkGraphWeightFilter : public svtkGraphAlgorithm
{
public:
  svtkTypeMacro(svtkGraphWeightFilter, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkGraphWeightFilter() {}
  ~svtkGraphWeightFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Compute the weight on the 'graph' for a particular 'edge'.
   * This is a pure virtual function that must be implemented in subclasses.
   */
  virtual float ComputeWeight(svtkGraph* const graph, const svtkEdgeType& edge) const = 0;

  /**
   * Ensure that the 'graph' is has all properties that are needed to compute
   * the weights. For example, in svtkGraphWeightEuclideanDistanceFilter,
   * 'graph' must have Points set for each vertex, as the ComputeWeight
   * function calls GetPoint.
   */
  virtual bool CheckRequirements(svtkGraph* const graph) const;

private:
  svtkGraphWeightFilter(const svtkGraphWeightFilter&) = delete;
  void operator=(const svtkGraphWeightFilter&) = delete;
};

#endif
