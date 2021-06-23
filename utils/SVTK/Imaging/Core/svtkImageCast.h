/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCast.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageCast
 * @brief    Image Data type Casting Filter
 *
 * svtkImageCast filter casts the input type to match the output type in
 * the image processing pipeline.  The filter does nothing if the input
 * already has the correct type.  To specify the "CastTo" type,
 * use "SetOutputScalarType" method.
 *
 * @warning
 * As svtkImageCast only casts values without rescaling them, its use is not
 * recommended. svtkImageShiftScale is the recommended way to change the type
 * of an image data.
 *
 * @sa
 * svtkImageThreshold svtkImageShiftScale
 */

#ifndef svtkImageCast_h
#define svtkImageCast_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageCast : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageCast* New();
  svtkTypeMacro(svtkImageCast, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the desired output scalar type to cast to.
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

  //@{
  /**
   * When the ClampOverflow flag is on, the data is thresholded so that
   * the output value does not exceed the max or min of the data type.
   * Clamping is safer because otherwise you might invoke undefined
   * behavior (and may crash) if the type conversion is out of range
   * of the data type.  On the other hand, clamping is slower.
   * By default ClampOverflow is off.
   */
  svtkSetMacro(ClampOverflow, svtkTypeBool);
  svtkGetMacro(ClampOverflow, svtkTypeBool);
  svtkBooleanMacro(ClampOverflow, svtkTypeBool);
  //@}

protected:
  svtkImageCast();
  ~svtkImageCast() override {}

  svtkTypeBool ClampOverflow;
  int OutputScalarType;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageCast(const svtkImageCast&) = delete;
  void operator=(const svtkImageCast&) = delete;
};

#endif
