/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataLIC2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDataLIC2D
 *
 *
 *  GPU implementation of a Line Integral Convolution, a technique for
 *  imaging vector fields.
 *
 *  The input on port 0 is an svtkImageData with extents of a 2D image. It needs
 *  a vector field on point data. This filter only works on point vectors. One
 *  can use a svtkCellDataToPointData filter to convert cell vectors to point
 *  vectors.
 *
 *  Port 1 is a special port for customized noise input. It is an optional port.
 *  If noise input is not specified, then the filter using svtkImageNoiseSource to
 *  generate a 128x128 noise texture.
 *
 * @sa
 *  svtkSurfaceLICPainter svtkLineIntegralConvolution2D
 */

#ifndef svtkImageDataLIC2D_h
#define svtkImageDataLIC2D_h

#include "svtkImageAlgorithm.h"
#include "svtkRenderingLICOpenGL2Module.h" // For export macro
#include "svtkWeakPointer.h"               // needed for svtkWeakPointer.

class svtkRenderWindow;
class svtkOpenGLRenderWindow;
class svtkImageNoiseSource;
class svtkImageCast;

class SVTKRENDERINGLICOPENGL2_EXPORT svtkImageDataLIC2D : public svtkImageAlgorithm
{
public:
  static svtkImageDataLIC2D* New();
  svtkTypeMacro(svtkImageDataLIC2D, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the context. Context must be a svtkOpenGLRenderWindow.
   * This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions. Return 0 upon failure and 1 upon success.
   */
  int SetContext(svtkRenderWindow* context);
  svtkRenderWindow* GetContext();
  //@}

  //@{
  /**
   * Number of steps. Initial value is 20.
   * class invariant: Steps>0.
   * In term of visual quality, the greater the better.
   */
  svtkSetMacro(Steps, int);
  svtkGetMacro(Steps, int);
  //@}

  //@{
  /**
   * Step size.
   * Specify the step size as a unit of the cell length of the input vector
   * field. Cell length is the length of the diagonal of a cell.
   * Initial value is 1.0.
   * class invariant: StepSize>0.0.
   * In term of visual quality, the smaller the better.
   * The type for the interface is double as SVTK interface is double
   * but GPU only supports float. This value will be converted to
   * float in the execution of the algorithm.
   */
  svtkSetMacro(StepSize, double);
  svtkGetMacro(StepSize, double);
  //@}

  //@{
  /**
   * The magnification factor. Default is 1
   */
  svtkSetMacro(Magnification, int);
  svtkGetMacro(Magnification, int);
  //@}

  //@{
  /**
   * Check if the required OpenGL extensions / GPU are supported.
   */
  svtkGetMacro(OpenGLExtensionsSupported, int);
  //@}

  void TranslateInputExtent(const int* inExt, const int* inWholeExtent, int* outExt);

protected:
  svtkImageDataLIC2D();
  ~svtkImageDataLIC2D() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Fill the input port information objects for this algorithm.  This
   * is invoked by the first call to GetInputPortInformation for each
   * port so subclasses can specify what they can handle.
   * Redefined from the superclass.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestUpdateExtent(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
    svtkInformationVector* svtkNotUsed(outputVector)) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkWeakPointer<svtkOpenGLRenderWindow> Context;
  bool OwnWindow;
  int OpenGLExtensionsSupported;

  svtkImageNoiseSource* NoiseSource;
  svtkImageCast* ImageCast;

  int Steps;
  double StepSize;
  int Magnification;

private:
  svtkImageDataLIC2D(const svtkImageDataLIC2D&) = delete;
  void operator=(const svtkImageDataLIC2D&) = delete;
};

#endif
