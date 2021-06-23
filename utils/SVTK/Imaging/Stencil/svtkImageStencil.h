/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageStencil
 * @brief   combine images via a cookie-cutter operation
 *
 * svtkImageStencil will combine two images together using a stencil.
 * The stencil should be provided in the form of a svtkImageStencilData,
 */

#ifndef svtkImageStencil_h
#define svtkImageStencil_h

#include "svtkImagingStencilModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkImageStencilData;

class SVTKIMAGINGSTENCIL_EXPORT svtkImageStencil : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageStencil* New();
  svtkTypeMacro(svtkImageStencil, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the stencil to use.  The stencil can be created
   * from a svtkImplicitFunction or a svtkPolyData. This
   * function does not setup a pipeline connection.
   */
  virtual void SetStencilData(svtkImageStencilData* stencil);
  svtkImageStencilData* GetStencil();
  //@}

  /**
   * Specify the stencil to use. This sets up a pipeline connection.
   */
  void SetStencilConnection(svtkAlgorithmOutput* outputPort)
  {
    this->SetInputConnection(2, outputPort);
  }

  //@{
  /**
   * Reverse the stencil.
   */
  svtkSetMacro(ReverseStencil, svtkTypeBool);
  svtkBooleanMacro(ReverseStencil, svtkTypeBool);
  svtkGetMacro(ReverseStencil, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the second input.  This image will be used for the 'outside' of the
   * stencil.  If not set, the output voxels will be filled with
   * BackgroundValue instead.
   */
  virtual void SetBackgroundInputData(svtkImageData* input);
  svtkImageData* GetBackgroundInput();
  //@}

  //@{
  /**
   * Set the default output value to use when the second input is not set.
   */
  void SetBackgroundValue(double val) { this->SetBackgroundColor(val, val, val, val); }
  double GetBackgroundValue() { return this->BackgroundColor[0]; }
  //@}

  //@{
  /**
   * Set the default color to use when the second input is not set.
   * This is like SetBackgroundValue, but for multi-component images.
   */
  svtkSetVector4Macro(BackgroundColor, double);
  svtkGetVector4Macro(BackgroundColor, double);
  //@}

protected:
  svtkImageStencil();
  ~svtkImageStencil() override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;

  svtkTypeBool ReverseStencil;
  double BackgroundColor[4];

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkImageStencil(const svtkImageStencil&) = delete;
  void operator=(const svtkImageStencil&) = delete;
};

#endif
