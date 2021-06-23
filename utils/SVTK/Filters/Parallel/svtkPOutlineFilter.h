/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPOutlineFilter
 * @brief   create wireframe outline for arbitrary data set
 *
 * svtkPOutlineFilter works like svtkOutlineFilter, but it looks for data
 * partitions in other processes.  It assumes the filter is operated
 * in a data parallel pipeline.
 */

#ifndef svtkPOutlineFilter_h
#define svtkPOutlineFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"
class svtkOutlineSource;
class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPOutlineFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkPOutlineFilter* New();
  svtkTypeMacro(svtkPOutlineFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPOutlineFilter();
  ~svtkPOutlineFilter() override;

  svtkMultiProcessController* Controller;
  svtkOutlineSource* OutlineSource;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPOutlineFilter(const svtkPOutlineFilter&) = delete;
  void operator=(const svtkPOutlineFilter&) = delete;
};
#endif
