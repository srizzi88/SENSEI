/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIYAggregateDataSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDIYAggregateDataSetFilter
 * @brief   Aggregates data sets to a reduced number of processes.
 *
 * This class allows svtkDataSets to be aggregated over a smaller set of processes.
 */

#ifndef svtkDIYAggregateDataSetFilter_h
#define svtkDIYAggregateDataSetFilter_h

#include "svtkAggregateDataSetFilter.h"
#include "svtkFiltersParallelDIY2Module.h" // For export macro

#include <map>    // For passing computed data between methods
#include <string> // For passing computed data between methods
#include <vector> // For passing computed data between methods

class svtkDataObject;
class svtkIdList;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkDIYAggregateDataSetFilter : public svtkAggregateDataSetFilter
{
  svtkTypeMacro(svtkDIYAggregateDataSetFilter, svtkAggregateDataSetFilter);

public:
  static svtkDIYAggregateDataSetFilter* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkDIYAggregateDataSetFilter();
  ~svtkDIYAggregateDataSetFilter() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Given a source process id and number of processes, return a target process id for the aggregate
   * operation. The target process id ranges from 0 to NumberOfTargetProcesses-1.
   * For source process ids that don't have a target process id, a -1 will be
   * returned.
   */
  int GetTargetProcessId(int sourceProcessId, int numberOfProcesses);

  /**
   * Given two extents and dimensions (marking whether or not we have cells
   * in that dimension with a value of 0 for no), return whether or not the
   * extents overlap by at least a single cell. We ignore if they share a
   * point as this is not needed to share information. If the extents do
   * overlap then the overlapping extent is returned in overlappingExtent
   * if it is non-null.
   */
  bool DoExtentsOverlap(int extent1[6], int extent2[6], int dimensions[3], int* overlappingExtent);

  /**
   * Get the extent of the topologically regular dataset.
   */
  void GetExtent(svtkDataSet* dataSet, int extent[6]);

  /**
   * Extract information from source dataset into target dataset.
   */
  void ExtractDataSetInformation(svtkDataSet* source, svtkDataSet* target);

  /**
   * Move data with DIY. Having issues with the serialized XML string.
   * so saving for later use.
   */
  int MoveDataWithDIY(int inputExtent[6], int wholeExtent[6], int outputExtent[6],
    std::map<int, std::string>& serializedDataSets, std::vector<std::string>& receivedDataSets);

  /**
   * Move data directly with svtkMPIController.
   */
  int MoveData(int inputExtent[6], int wholeExtent[6], int outputExtent[6],
    std::map<int, std::string>& serializedDataSets, std::vector<std::string>& receivedDataSets);

  /**
   * Determine which processes I receive data and put those process ranks (in order)
   * into *processesIReceiveFrom*.
   */
  void ComputeProcessesIReceiveFrom(
    int inputExent[6], int wholeExtent[6], int outputExtent[6], svtkIdList* processesIReceiveFrom);

  /**
   * Put appropriate values from *sourceCoordinates* into *targetCoordinates*
   * based on the extents overlap.
   */
  void ExtractRectilinearGridCoordinates(int* sourceExtent, int* targetExtent,
    svtkDataArray* sourceCoordinates, svtkDataArray* targetCoordinates);

private:
  svtkDIYAggregateDataSetFilter(const svtkDIYAggregateDataSetFilter&) = delete;
  void operator=(const svtkDIYAggregateDataSetFilter&) = delete;

  /**
   * Used to keep track of whether or not we've initialized the output
   * dataset
   */
  bool OutputInitialized;
};

#endif
