/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageInPlaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageInPlaceFilter
 * @brief   Filter that operates in place.
 *
 * svtkImageInPlaceFilter is a filter super class that
 * operates directly on the input region.  The data is copied
 * if the requested region has different extent than the input region
 * or some other object is referencing the input region.
 */

#ifndef svtkImageInPlaceFilter_h
#define svtkImageInPlaceFilter_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkImageInPlaceFilter : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkImageInPlaceFilter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkImageInPlaceFilter();
  ~svtkImageInPlaceFilter() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  void CopyData(svtkImageData* in, svtkImageData* out, int* outExt);

private:
  svtkImageInPlaceFilter(const svtkImageInPlaceFilter&) = delete;
  void operator=(const svtkImageInPlaceFilter&) = delete;
};

#endif
