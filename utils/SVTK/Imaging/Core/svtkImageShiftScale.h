/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageShiftScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageShiftScale
 * @brief   shift and scale an input image
 *
 * With svtkImageShiftScale Pixels are shifted (a constant value added)
 * and then scaled (multiplied by a scalar. As a convenience, this class
 * allows you to set the output scalar type similar to svtkImageCast.
 * This is because shift scale operations frequently convert data types.
 */

#ifndef svtkImageShiftScale_h
#define svtkImageShiftScale_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageShiftScale : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageShiftScale* New();
  svtkTypeMacro(svtkImageShiftScale, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the shift value. This value is added to each pixel
   */
  svtkSetMacro(Shift, double);
  svtkGetMacro(Shift, double);
  //@}

  //@{
  /**
   * Set/Get the scale value. Each pixel is multiplied by this value.
   */
  svtkSetMacro(Scale, double);
  svtkGetMacro(Scale, double);
  //@}

  //@{
  /**
   * Set the desired output scalar type. The result of the shift
   * and scale operations is cast to the type specified.
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
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(SVTK_UNSIGNED_CHAR); }
  //@}

  //@{
  /**
   * When the ClampOverflow flag is on, the data is thresholded so that
   * the output value does not exceed the max or min of the data type.
   * Clamping is safer because otherwise you might invoke undefined
   * behavior (and may crash) if the type conversion is out of range
   * of the data type.  On the other hand, clamping is slower.
   * By default, ClampOverflow is off.
   */
  svtkSetMacro(ClampOverflow, svtkTypeBool);
  svtkGetMacro(ClampOverflow, svtkTypeBool);
  svtkBooleanMacro(ClampOverflow, svtkTypeBool);
  //@}

protected:
  svtkImageShiftScale();
  ~svtkImageShiftScale() override;

  double Shift;
  double Scale;
  int OutputScalarType;
  svtkTypeBool ClampOverflow;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*,
    svtkImageData*** inData, svtkImageData** outData, int outExt[6], int threadId) override;

private:
  svtkImageShiftScale(const svtkImageShiftScale&) = delete;
  void operator=(const svtkImageShiftScale&) = delete;
};

#endif
