/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLImageGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLImageGradient
 * @brief   Compute Gradient using the GPU
 */

#ifndef svtkOpenGLImageGradient_h
#define svtkOpenGLImageGradient_h

#include "svtkImageGradient.h"
#include "svtkImagingOpenGL2Module.h" // For export macro

class svtkOpenGLImageAlgorithmHelper;
class svtkRenderWindow;

class SVTKIMAGINGOPENGL2_EXPORT svtkOpenGLImageGradient : public svtkImageGradient
{
public:
  static svtkOpenGLImageGradient* New();
  svtkTypeMacro(svtkOpenGLImageGradient, svtkImageGradient);

  /**
   * Set the render window to get the OpenGL resources from
   */
  void SetRenderWindow(svtkRenderWindow*);

protected:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkOpenGLImageGradient();
  ~svtkOpenGLImageGradient() override;

  svtkOpenGLImageAlgorithmHelper* Helper;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;

private:
  svtkOpenGLImageGradient(const svtkOpenGLImageGradient&) = delete;
  void operator=(const svtkOpenGLImageGradient&) = delete;
};

#endif
