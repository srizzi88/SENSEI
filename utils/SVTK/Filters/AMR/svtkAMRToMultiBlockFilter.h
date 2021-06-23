/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRToMultiBlockFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRToMultiBlockFilter
 *
 *
 * A filter that accepts as input an AMR dataset and produces a corresponding
 * svtkMultiBlockDataset as output.
 *
 * @sa
 * svtkOverlappingAMR svtkMultiBlockDataSet
 */

#ifndef svtkAMRToMultiBlockFilter_h
#define svtkAMRToMultiBlockFilter_h

#include "svtkFiltersAMRModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkInformation;
class svtkInformationVector;
class svtkIndent;
class svtkMultiProcessController;
class svtkOverlappingAMR;
class svtkMultiBlockDataSet;

class SVTKFILTERSAMR_EXPORT svtkAMRToMultiBlockFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkAMRToMultiBlockFilter* New();
  svtkTypeMacro(svtkAMRToMultiBlockFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& oss, svtkIndent indent) override;

  //@{
  /**
   * Set/Get a multiprocess controller for paralle processing.
   * By default this parameter is set to nullptr by the constructor.
   */
  svtkSetMacro(Controller, svtkMultiProcessController*);
  svtkGetMacro(Controller, svtkMultiProcessController*);
  //@}

  // Standard pipeline routines

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkAMRToMultiBlockFilter();
  ~svtkAMRToMultiBlockFilter() override;

  //@{
  /**
   * Copies the AMR data to the output multi-block datastructure.
   */
  void CopyAMRToMultiBlock(svtkOverlappingAMR* amr, svtkMultiBlockDataSet* mbds);
  svtkMultiProcessController* Controller;
  //@}

private:
  svtkAMRToMultiBlockFilter(const svtkAMRToMultiBlockFilter&) = delete;
  void operator=(const svtkAMRToMultiBlockFilter&) = delete;
};

#endif /* svtkAMRToMultiBlockFilter_h */
