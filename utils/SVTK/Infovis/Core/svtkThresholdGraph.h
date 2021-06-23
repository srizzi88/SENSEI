/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollapseVerticesByArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkThresholdGraph
 * @brief   Returns a subgraph of a svtkGraph.
 *
 *
 * Requires input array, lower and upper threshold. This filter than
 * extracts the subgraph based on these three parameters.
 */

#ifndef svtkThresholdGraph_h
#define svtkThresholdGraph_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkThresholdGraph : public svtkGraphAlgorithm
{
public:
  static svtkThresholdGraph* New();
  svtkTypeMacro(svtkThresholdGraph, svtkGraphAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set lower threshold. This would be the value against which
   * edge or vertex data array value will be compared.
   */
  svtkGetMacro(LowerThreshold, double);
  svtkSetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Get/Set upper threshold. This would be the value against which
   * edge or vertex data array value will be compared.
   */
  svtkGetMacro(UpperThreshold, double);
  svtkSetMacro(UpperThreshold, double);
  //@}

protected:
  svtkThresholdGraph();
  ~svtkThresholdGraph() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  double LowerThreshold;
  double UpperThreshold;

  svtkThresholdGraph(const svtkThresholdGraph&) = delete;
  void operator=(const svtkThresholdGraph&) = delete;
};

#endif // svtkThresholdGraph_h
