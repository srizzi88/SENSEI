/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCachedStreamingDemandDrivenPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCachedStreamingDemandDrivenPipeline
 *
 * svtkCachedStreamingDemandDrivenPipeline
 */

#ifndef svtkCachedStreamingDemandDrivenPipeline_h
#define svtkCachedStreamingDemandDrivenPipeline_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkStreamingDemandDrivenPipeline.h"

class svtkInformationIntegerKey;
class svtkInformationIntegerVectorKey;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkCachedStreamingDemandDrivenPipeline
  : public svtkStreamingDemandDrivenPipeline
{
public:
  static svtkCachedStreamingDemandDrivenPipeline* New();
  svtkTypeMacro(svtkCachedStreamingDemandDrivenPipeline, svtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This is the maximum number of images that can be retained in memory.
   * it defaults to 10.
   */
  void SetCacheSize(int size);
  svtkGetMacro(CacheSize, int);
  //@}

protected:
  svtkCachedStreamingDemandDrivenPipeline();
  ~svtkCachedStreamingDemandDrivenPipeline() override;

  int NeedToExecuteData(
    int outputPort, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec) override;
  int ExecuteData(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec) override;

  int CacheSize;

  svtkDataObject** Data;
  svtkMTimeType* Times;

private:
  svtkCachedStreamingDemandDrivenPipeline(const svtkCachedStreamingDemandDrivenPipeline&) = delete;
  void operator=(const svtkCachedStreamingDemandDrivenPipeline&) = delete;
};

#endif
