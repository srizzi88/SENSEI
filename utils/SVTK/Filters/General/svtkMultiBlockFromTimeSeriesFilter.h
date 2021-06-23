/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockFromTimeSeriesFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiBlockFromTimeSeriesFilter
 * @brief   collects multiple inputs into one multi-group dataset
 *
 * svtkMultiBlockFromTimeSeriesFilter is a 1 to 1 filter that merges multiple
 * time steps from the input into one multiblock dataset.  It will assign each
 * time step from the input to one group of the multi-block dataset and will
 * assign each timestep's data as a block in the multi-block datset.
 */

#ifndef svtkMultiBlockFromTimeSeriesFilter_h
#define svtkMultiBlockFromTimeSeriesFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkSmartPointer.h" // Smart pointer

#include <vector> // Vector to hold timesteps

class svtkMultiBlockDataSet;

class SVTKFILTERSGENERAL_EXPORT svtkMultiBlockFromTimeSeriesFilter
  : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkMultiBlockFromTimeSeriesFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMultiBlockFromTimeSeriesFilter* New();

protected:
  svtkMultiBlockFromTimeSeriesFilter();
  ~svtkMultiBlockFromTimeSeriesFilter() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMultiBlockFromTimeSeriesFilter(const svtkMultiBlockFromTimeSeriesFilter&) = delete;
  void operator=(const svtkMultiBlockFromTimeSeriesFilter&) = delete;

  int UpdateTimeIndex;
  std::vector<double> TimeSteps;
  svtkSmartPointer<svtkMultiBlockDataSet> TempDataset;
};

#endif
