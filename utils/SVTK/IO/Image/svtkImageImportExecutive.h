/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageImportExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageImportExecutive
 *
 * svtkImageImportExecutive
 */

#ifndef svtkImageImportExecutive_h
#define svtkImageImportExecutive_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkStreamingDemandDrivenPipeline.h"

class SVTKIOIMAGE_EXPORT svtkImageImportExecutive : public svtkStreamingDemandDrivenPipeline
{
public:
  static svtkImageImportExecutive* New();
  svtkTypeMacro(svtkImageImportExecutive, svtkStreamingDemandDrivenPipeline);

  /**
   * Override to implement some requests with callbacks.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

protected:
  svtkImageImportExecutive() {}
  ~svtkImageImportExecutive() override {}

private:
  svtkImageImportExecutive(const svtkImageImportExecutive&) = delete;
  void operator=(const svtkImageImportExecutive&) = delete;
};

#endif
