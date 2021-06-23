/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReebGraphSimplificationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReebGraphSimplificationFilter
 * @brief   simplify an input Reeb graph.
 *
 * The filter takes an input svtkReebGraph object and outputs a
 * svtkReebGraph object.
 */

#ifndef svtkReebGraphSimplificationFilter_h
#define svtkReebGraphSimplificationFilter_h

#include "svtkDirectedGraphAlgorithm.h"
#include "svtkFiltersReebGraphModule.h" // For export macro

class svtkReebGraph;
class svtkReebGraphSimplificationMetric;

class SVTKFILTERSREEBGRAPH_EXPORT svtkReebGraphSimplificationFilter : public svtkDirectedGraphAlgorithm
{
public:
  static svtkReebGraphSimplificationFilter* New();
  svtkTypeMacro(svtkReebGraphSimplificationFilter, svtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the persistence threshold for simplification (from 0 to 1).
   * Default value: 0 (no simplification).
   */
  svtkSetMacro(SimplificationThreshold, double);
  svtkGetMacro(SimplificationThreshold, double);
  //@}

  /**
   * Set the persistence metric evaluation code
   * Default value: nullptr (standard topological persistence).
   */
  void SetSimplificationMetric(svtkReebGraphSimplificationMetric* metric);

  svtkReebGraph* GetOutput();

protected:
  svtkReebGraphSimplificationFilter();
  ~svtkReebGraphSimplificationFilter() override;

  double SimplificationThreshold;

  svtkReebGraphSimplificationMetric* SimplificationMetric;

  int FillInputPortInformation(int portNumber, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkReebGraphSimplificationFilter(const svtkReebGraphSimplificationFilter&) = delete;
  void operator=(const svtkReebGraphSimplificationFilter&) = delete;
};

#endif
