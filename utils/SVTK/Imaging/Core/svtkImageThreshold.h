/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageThreshold
 * @brief    Flexible threshold
 *
 * svtkImageThreshold can do binary or continuous thresholding for lower, upper
 * or a range of data.  The output data type may be different than the
 * output, but defaults to the same type.
 */

#ifndef svtkImageThreshold_h
#define svtkImageThreshold_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageThreshold : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageThreshold* New();
  svtkTypeMacro(svtkImageThreshold, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The values greater than or equal to the value match.
   */
  void ThresholdByUpper(double thresh);

  /**
   * The values less than or equal to the value match.
   */
  void ThresholdByLower(double thresh);

  /**
   * The values in a range (inclusive) match
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Determines whether to replace the pixel in range with InValue
   */
  svtkSetMacro(ReplaceIn, svtkTypeBool);
  svtkGetMacro(ReplaceIn, svtkTypeBool);
  svtkBooleanMacro(ReplaceIn, svtkTypeBool);
  //@}

  //@{
  /**
   * Replace the in range pixels with this value.
   */
  void SetInValue(double val);
  svtkGetMacro(InValue, double);
  //@}

  //@{
  /**
   * Determines whether to replace the pixel out of range with OutValue
   */
  svtkSetMacro(ReplaceOut, svtkTypeBool);
  svtkGetMacro(ReplaceOut, svtkTypeBool);
  svtkBooleanMacro(ReplaceOut, svtkTypeBool);
  //@}

  //@{
  /**
   * Replace the in range pixels with this value.
   */
  void SetOutValue(double val);
  svtkGetMacro(OutValue, double);
  //@}

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  svtkGetMacro(UpperThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Set the desired output scalar type to cast to
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
  svtkImageThreshold();
  ~svtkImageThreshold() override {}

  double UpperThreshold;
  double LowerThreshold;
  svtkTypeBool ReplaceIn;
  double InValue;
  svtkTypeBool ReplaceOut;
  double OutValue;

  int OutputScalarType;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;

private:
  svtkImageThreshold(const svtkImageThreshold&) = delete;
  void operator=(const svtkImageThreshold&) = delete;
};

#endif
