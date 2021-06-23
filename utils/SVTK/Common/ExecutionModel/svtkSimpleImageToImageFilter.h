/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleImageToImageFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimpleImageToImageFilter
 * @brief   Generic image filter with one input.
 *
 * svtkSimpleImageToImageFilter is a filter which aims to avoid much
 * of the complexity associated with svtkImageAlgorithm (i.e.
 * support for pieces, multi-threaded operation). If you need to write
 * a simple image-image filter which operates on the whole input, use
 * this as the superclass. The subclass has to provide only an execute
 * method which takes input and output as arguments. Memory allocation
 * is handled in svtkSimpleImageToImageFilter. Also, you are guaranteed to
 * have a valid input in the Execute(input, output) method. By default,
 * this filter
 * requests it's input's whole extent and copies the input's information
 * (spacing, whole extent etc...) to the output. If the output's setup
 * is different (for example, if it performs some sort of sub-sampling),
 * ExecuteInformation has to be overwritten. As an example of how this
 * can be done, you can look at svtkImageShrink3D::ExecuteInformation.
 * For a complete example which uses templates to support generic data
 * types, see svtkSimpleImageToImageFilter.
 *
 * @sa
 * svtkImageAlgorithm svtkSimpleImageFilterExample
 */

#ifndef svtkSimpleImageToImageFilter_h
#define svtkSimpleImageToImageFilter_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkSimpleImageToImageFilter : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkSimpleImageToImageFilter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkSimpleImageToImageFilter();
  ~svtkSimpleImageToImageFilter() override;

  // These are called by the superclass.
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // You don't have to touch this unless you have a good reason.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // In the simplest case, this is the only method you need to define.
  virtual void SimpleExecute(svtkImageData* input, svtkImageData* output) = 0;

private:
  svtkSimpleImageToImageFilter(const svtkSimpleImageToImageFilter&) = delete;
  void operator=(const svtkSimpleImageToImageFilter&) = delete;
};

#endif
