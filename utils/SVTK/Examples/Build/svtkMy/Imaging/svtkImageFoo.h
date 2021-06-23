/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageFoo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageFoo
 * @brief   foo and scale an input image
 *
 * With svtkImageFoo Pixels are foo'ed.
 */

#ifndef svtkImageFoo_h
#define svtkImageFoo_h

#include "svtkThreadedImageAlgorithm.h"
#include "svtkmyImagingModule.h" // For export macro

class svtkBar;

class SVTKMYIMAGING_EXPORT svtkImageFoo : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageFoo* New();
  svtkTypeMacro(svtkImageFoo, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the foo value.
   */
  svtkSetMacro(Foo, float);
  svtkGetMacro(Foo, float);
  //@}

  //@{
  /**
   * Set the desired output scalar type.
   */
  svtkSetMacro(OutputScalarType, int);
  svtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(SVTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(SVTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(SVTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(SVTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(SVTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(SVTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(SVTK_CHAR); }
  void SetOutputScalarTypeToSignedChar() { this->SetOutputScalarType(SVTK_SIGNED_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(SVTK_UNSIGNED_CHAR); }
  //@}

protected:
  svtkImageFoo();
  ~svtkImageFoo() override;

  float Foo;
  int OutputScalarType;
  svtkBar* Bar;

  int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;

private:
  svtkImageFoo(const svtkImageFoo&) = delete;
  void operator=(const svtkImageFoo&) = delete;
};

#endif
