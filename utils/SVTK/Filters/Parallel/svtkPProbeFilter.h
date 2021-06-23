/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPProbeFilter
 * @brief   probe dataset in parallel
 *
 * This filter works correctly only if the whole geometry dataset
 * (that specify the point locations used to probe input) is available on all
 * nodes.
 */

#ifndef svtkPProbeFilter_h
#define svtkPProbeFilter_h

#include "svtkCompositeDataProbeFilter.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPProbeFilter : public svtkCompositeDataProbeFilter
{
public:
  svtkTypeMacro(svtkPProbeFilter, svtkCompositeDataProbeFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPProbeFilter* New();

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPProbeFilter();
  ~svtkPProbeFilter() override;

  enum
  {
    PROBE_COMMUNICATION_TAG = 1970
  };

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkMultiProcessController* Controller;

private:
  svtkPProbeFilter(const svtkPProbeFilter&) = delete;
  void operator=(const svtkPProbeFilter&) = delete;
};

#endif
