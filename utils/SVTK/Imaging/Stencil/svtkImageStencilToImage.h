/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageStencilToImage
 * @brief   Convert an image stencil into an image
 *
 * svtkImageStencilToImage will convert an image stencil into a binary
 * image.  The default output will be an 8-bit image with a value of 1
 * inside the stencil and 0 outside.  When used in combination with
 * svtkPolyDataToImageStencil or svtkImplicitFunctionToImageStencil, this
 * can be used to create a binary image from a mesh or a function.
 * @sa
 * svtkImplicitModeller
 */

#ifndef svtkImageStencilToImage_h
#define svtkImageStencilToImage_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingStencilModule.h" // For export macro

class SVTKIMAGINGSTENCIL_EXPORT svtkImageStencilToImage : public svtkImageAlgorithm
{
public:
  static svtkImageStencilToImage* New();
  svtkTypeMacro(svtkImageStencilToImage, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The value to use outside the stencil.  The default is 0.
   */
  svtkSetMacro(OutsideValue, double);
  svtkGetMacro(OutsideValue, double);
  //@}

  //@{
  /**
   * The value to use inside the stencil.  The default is 1.
   */
  svtkSetMacro(InsideValue, double);
  svtkGetMacro(InsideValue, double);
  //@}

  //@{
  /**
   * The desired output scalar type.  The default is unsigned char.
   */
  svtkSetMacro(OutputScalarType, int);
  svtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(SVTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(SVTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(SVTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(SVTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(SVTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(SVTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(SVTK_UNSIGNED_CHAR); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(SVTK_CHAR); }
  //@}

protected:
  svtkImageStencilToImage();
  ~svtkImageStencilToImage() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double OutsideValue;
  double InsideValue;
  int OutputScalarType;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkImageStencilToImage(const svtkImageStencilToImage&) = delete;
  void operator=(const svtkImageStencilToImage&) = delete;
};

#endif
