/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageResliceToColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageResliceToColors
 * @brief   Reslice and produce color scalars.
 *
 * svtkImageResliceToColors is an extension of svtkImageReslice that
 * produces color scalars.  It should be provided with a lookup table
 * that defines the output colors and the desired range of input values
 * to map to those colors.  If the input has multiple components, then
 * you should use the SetVectorMode() method of the lookup table to
 * specify how the vectors will be colored.  If no lookup table is
 * provided, then the input must already be color scalars, but they
 * will be converted to the specified output format.
 * @sa
 * svtkImageMapToColors
 */

#ifndef svtkImageResliceToColors_h
#define svtkImageResliceToColors_h

#include "svtkImageReslice.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkScalarsToColors;

class SVTKIMAGINGCORE_EXPORT svtkImageResliceToColors : public svtkImageReslice
{
public:
  static svtkImageResliceToColors* New();
  svtkTypeMacro(svtkImageResliceToColors, svtkImageReslice);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set a lookup table to apply to the data.  Use the Range,
   * VectorMode, and VectorComponents of the table to control
   * the mapping of the input data to colors.  If any output
   * voxel is transformed to a point outside the input volume,
   * then that voxel will be set to the BackgroundColor.
   */
  virtual void SetLookupTable(svtkScalarsToColors* table);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Set the output format, the default is RGBA.
   */
  svtkSetClampMacro(OutputFormat, int, SVTK_LUMINANCE, SVTK_RGBA);
  svtkGetMacro(OutputFormat, int);
  void SetOutputFormatToRGBA() { this->OutputFormat = SVTK_RGBA; }
  void SetOutputFormatToRGB() { this->OutputFormat = SVTK_RGB; }
  void SetOutputFormatToLuminanceAlpha() { this->OutputFormat = SVTK_LUMINANCE_ALPHA; }
  void SetOutputFormatToLuminance() { this->OutputFormat = SVTK_LUMINANCE; }
  //@}

  /**
   * Bypass the color mapping operation and output the scalar
   * values directly.  The output values will be float, rather
   * than the input data type.
   */
  void SetBypass(int bypass);
  void BypassOn() { this->SetBypass(1); }
  void BypassOff() { this->SetBypass(0); }
  int GetBypass() { return this->Bypass; }

  /**
   * When determining the modified time of the filter,
   * this checks the modified time of the transform and matrix.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkImageResliceToColors();
  ~svtkImageResliceToColors() override;

  svtkScalarsToColors* LookupTable;
  svtkScalarsToColors* DefaultLookupTable;
  int OutputFormat;
  int Bypass;

  int ConvertScalarInfo(int& scalarType, int& numComponents) override;

  void ConvertScalars(void* inPtr, void* outPtr, int inputType, int inputNumComponents, int count,
    int idX, int idY, int idZ, int threadId) override;

private:
  svtkImageResliceToColors(const svtkImageResliceToColors&) = delete;
  void operator=(const svtkImageResliceToColors&) = delete;
};

#endif
