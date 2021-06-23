/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkForceTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkForceTime_h
#define svtkForceTime_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKFILTERSHYBRID_EXPORT svtkForceTime : public svtkPassInputTypeAlgorithm
{
public:
  static svtkForceTime* New();
  svtkTypeMacro(svtkForceTime, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description:
  // Replace the pipeline time by this one.
  svtkSetMacro(ForcedTime, double);
  svtkGetMacro(ForcedTime, double);

  // Description:
  // Use the ForcedTime. If disabled, use usual pipeline time.
  svtkSetMacro(IgnorePipelineTime, bool);
  svtkGetMacro(IgnorePipelineTime, bool);
  svtkBooleanMacro(IgnorePipelineTime, bool);

protected:
  svtkForceTime();
  ~svtkForceTime() override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkForceTime(const svtkForceTime&) = delete;
  void operator=(const svtkForceTime&) = delete;

  double ForcedTime;
  bool IgnorePipelineTime;
  double PipelineTime;
  bool PipelineTimeFlag;
  svtkDataObject* Cache;
};

#endif // svtkForceTime_h
