/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReaderExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkReaderExecutive
 * @brief   Executive that works with svtkReaderAlgorithm and subclasses.
 *
 * svtkReaderExecutive is an executive that supports simplified API readers
 * that are written by subclassing from the svtkReaderAlgorithm hierarchy.
 * Currently, its main functionality is to call the basic reader API instead
 * if the standard ProcessRequest() method that other algorithms use.
 * In time, this is likely to add functionality such as caching. See
 * svtkReaderAlgorithm for the API.
 *
 * Note that this executive assumes that the reader has one output port.
 */

#ifndef svtkReaderExecutive_h
#define svtkReaderExecutive_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkStreamingDemandDrivenPipeline.h"

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkReaderExecutive : public svtkStreamingDemandDrivenPipeline
{
public:
  static svtkReaderExecutive* New();
  svtkTypeMacro(svtkReaderExecutive, svtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Overwritten to call the svtkReaderAlgorithm API instead of
   * ProcessRequest().
   */
  virtual int CallAlgorithm(svtkInformation* request, int direction, svtkInformationVector** inInfo,
    svtkInformationVector* outInfo) override;

protected:
  svtkReaderExecutive();
  ~svtkReaderExecutive() override;

private:
  svtkReaderExecutive(const svtkReaderExecutive&) = delete;
  void operator=(const svtkReaderExecutive&) = delete;
};

#endif
