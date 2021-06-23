/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapToColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMapToColors
 * @brief   map the input image through a lookup table
 *
 * The svtkImageMapToColors filter will take an input image of any valid
 * scalar type, and map the first component of the image through a
 * lookup table.  The result is an image of type SVTK_UNSIGNED_CHAR.
 * If the lookup table is not set, or is set to nullptr, then the input
 * data will be passed through if it is already of type SVTK_UNSIGNED_CHAR.
 *
 * @sa
 * svtkLookupTable svtkScalarsToColors
 */

#ifndef svtkImageMapToColors_h
#define svtkImageMapToColors_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkScalarsToColors;

class SVTKIMAGINGCORE_EXPORT svtkImageMapToColors : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageMapToColors* New();
  svtkTypeMacro(svtkImageMapToColors, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the lookup table.
   */
  virtual void SetLookupTable(svtkScalarsToColors*);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Set the output format, the default is RGBA.
   */
  svtkSetMacro(OutputFormat, int);
  svtkGetMacro(OutputFormat, int);
  void SetOutputFormatToRGBA() { this->OutputFormat = SVTK_RGBA; }
  void SetOutputFormatToRGB() { this->OutputFormat = SVTK_RGB; }
  void SetOutputFormatToLuminanceAlpha() { this->OutputFormat = SVTK_LUMINANCE_ALPHA; }
  void SetOutputFormatToLuminance() { this->OutputFormat = SVTK_LUMINANCE; }
  //@}

  //@{
  /**
   * Set the component to map for multi-component images (default: 0)
   */
  svtkSetMacro(ActiveComponent, int);
  svtkGetMacro(ActiveComponent, int);
  //@}

  //@{
  /**
   * Use the alpha component of the input when computing the alpha component
   * of the output (useful when converting monochrome+alpha data to RGBA)
   */
  svtkSetMacro(PassAlphaToOutput, svtkTypeBool);
  svtkBooleanMacro(PassAlphaToOutput, svtkTypeBool);
  svtkGetMacro(PassAlphaToOutput, svtkTypeBool);
  //@}

  /**
   * We need to check the modified time of the lookup table too.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get Color that should be used in case of UnMatching
   * data.
   */
  svtkSetVector4Macro(NaNColor, unsigned char);
  svtkGetVector4Macro(NaNColor, unsigned char);
  //@}

protected:
  svtkImageMapToColors();
  ~svtkImageMapToColors() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkScalarsToColors* LookupTable;
  int OutputFormat;

  int ActiveComponent;
  svtkTypeBool PassAlphaToOutput;

  int DataWasPassed;

  unsigned char NaNColor[4];

private:
  svtkImageMapToColors(const svtkImageMapToColors&) = delete;
  void operator=(const svtkImageMapToColors&) = delete;
};

#endif
