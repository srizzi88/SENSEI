/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitFunctionToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitFunctionToImageStencil
 * @brief   clip an image with a function
 *
 * svtkImplicitFunctionToImageStencil will convert a svtkImplicitFunction into
 * a stencil that can be used with svtkImageStencil or with other classes
 * that apply a stencil to an image.
 * @sa
 * svtkImplicitFunction svtkImageStencil svtkPolyDataToImageStencil
 */

#ifndef svtkImplicitFunctionToImageStencil_h
#define svtkImplicitFunctionToImageStencil_h

#include "svtkImageStencilSource.h"
#include "svtkImagingStencilModule.h" // For export macro

class svtkImplicitFunction;

class SVTKIMAGINGSTENCIL_EXPORT svtkImplicitFunctionToImageStencil : public svtkImageStencilSource
{
public:
  static svtkImplicitFunctionToImageStencil* New();
  svtkTypeMacro(svtkImplicitFunctionToImageStencil, svtkImageStencilSource);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the implicit function to convert into a stencil.
   */
  virtual void SetInput(svtkImplicitFunction*);
  svtkGetObjectMacro(Input, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Set the threshold value for the implicit function.
   */
  svtkSetMacro(Threshold, double);
  svtkGetMacro(Threshold, double);
  //@}

  /**
   * Override GetMTime() to account for the implicit function.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkImplicitFunctionToImageStencil();
  ~svtkImplicitFunctionToImageStencil() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkImplicitFunction* Input;
  double Threshold;

private:
  svtkImplicitFunctionToImageStencil(const svtkImplicitFunctionToImageStencil&) = delete;
  void operator=(const svtkImplicitFunctionToImageStencil&) = delete;
};

#endif
