/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAggregateDataSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAggregateDataSetFilter
 * @brief   Aggregates data sets to a reduced number of processes.
 *
 * This class allows polydata and unstructured grids to be aggregated
 * over a smaller set of processes. The derived svtkDIYAggregateDataSetFilter
 * will operate on image data, rectilinear grids and structured grids.
 */

#ifndef svtkAggregateDataSetFilter_h
#define svtkAggregateDataSetFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkAggregateDataSetFilter : public svtkPassInputTypeAlgorithm
{
public:
  static svtkAggregateDataSetFilter* New();
  svtkTypeMacro(svtkAggregateDataSetFilter, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Number of target processes. Valid values are between 1 and the total
   * number of processes. The default is 1. If a value is passed in that
   * is less than 1 than NumberOfTargetProcesses is changed/kept at 1.
   * If a value is passed in that is greater than the total number of
   * processes then NumberOfTargetProcesses is changed/kept at the
   * total number of processes. This is useful for scripting use cases
   * where later on the script is run with more processes than the
   * current amount.
   */
  void SetNumberOfTargetProcesses(int);
  svtkGetMacro(NumberOfTargetProcesses, int);
  //@}

protected:
  svtkAggregateDataSetFilter();
  ~svtkAggregateDataSetFilter() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int NumberOfTargetProcesses;

private:
  svtkAggregateDataSetFilter(const svtkAggregateDataSetFilter&) = delete;
  void operator=(const svtkAggregateDataSetFilter&) = delete;
};

#endif
