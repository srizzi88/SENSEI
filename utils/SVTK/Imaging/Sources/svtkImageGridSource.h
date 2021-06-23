/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageGridSource
 * @brief   Create an image of a grid.
 *
 * svtkImageGridSource produces an image of a grid.  The
 * default output type is double.
 */

#ifndef svtkImageGridSource_h
#define svtkImageGridSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingSourcesModule.h" // For export macro

class SVTKIMAGINGSOURCES_EXPORT svtkImageGridSource : public svtkImageAlgorithm
{
public:
  static svtkImageGridSource* New();
  svtkTypeMacro(svtkImageGridSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the grid spacing in pixel units.  Default (10,10,0).
   * A value of zero means no grid.
   */
  svtkSetVector3Macro(GridSpacing, int);
  svtkGetVector3Macro(GridSpacing, int);
  //@}

  //@{
  /**
   * Set/Get the grid origin, in ijk integer values.  Default (0,0,0).
   */
  svtkSetVector3Macro(GridOrigin, int);
  svtkGetVector3Macro(GridOrigin, int);
  //@}

  //@{
  /**
   * Set the grey level of the lines. Default 1.0.
   */
  svtkSetMacro(LineValue, double);
  svtkGetMacro(LineValue, double);
  //@}

  //@{
  /**
   * Set the grey level of the fill. Default 0.0.
   */
  svtkSetMacro(FillValue, double);
  svtkGetMacro(FillValue, double);
  //@}

  //@{
  /**
   * Set/Get the data type of pixels in the imported data.
   * As a convenience, the OutputScalarType is set to the same value.
   */
  svtkSetMacro(DataScalarType, int);
  void SetDataScalarTypeToDouble() { this->SetDataScalarType(SVTK_DOUBLE); }
  void SetDataScalarTypeToInt() { this->SetDataScalarType(SVTK_INT); }
  void SetDataScalarTypeToShort() { this->SetDataScalarType(SVTK_SHORT); }
  void SetDataScalarTypeToUnsignedShort() { this->SetDataScalarType(SVTK_UNSIGNED_SHORT); }
  void SetDataScalarTypeToUnsignedChar() { this->SetDataScalarType(SVTK_UNSIGNED_CHAR); }
  svtkGetMacro(DataScalarType, int);
  const char* GetDataScalarTypeAsString()
  {
    return svtkImageScalarTypeNameMacro(this->DataScalarType);
  }
  //@}

  //@{
  /**
   * Set/Get the extent of the whole output image,
   * Default: (0,255,0,255,0,0)
   */
  svtkSetVector6Macro(DataExtent, int);
  svtkGetVector6Macro(DataExtent, int);
  //@}

  //@{
  /**
   * Set/Get the pixel spacing.
   */
  svtkSetVector3Macro(DataSpacing, double);
  svtkGetVector3Macro(DataSpacing, double);
  //@}

  //@{
  /**
   * Set/Get the origin of the data.
   */
  svtkSetVector3Macro(DataOrigin, double);
  svtkGetVector3Macro(DataOrigin, double);
  //@}

protected:
  svtkImageGridSource();
  ~svtkImageGridSource() override {}

  int GridSpacing[3];
  int GridOrigin[3];

  double LineValue;
  double FillValue;

  int DataScalarType;

  int DataExtent[6];
  double DataSpacing[3];
  double DataOrigin[3];

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

private:
  svtkImageGridSource(const svtkImageGridSource&) = delete;
  void operator=(const svtkImageGridSource&) = delete;
};

#endif
