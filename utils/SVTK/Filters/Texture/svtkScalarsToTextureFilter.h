/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarsToTextureFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkScalarsToTextureFilter
 * @brief   generate texture coordinates and a texture image based on a scalar field
 *
 * This filter computes texture coordinates and a 2D texture image based on a polydata,
 * a color transfer function and an array.
 * The output port 0 will contain the input polydata with computed texture coordinates.
 * The output port 1 will contain the texture.
 * The computed texture coordinates is based on svtkTextureMapToPlane which computes them using
 * 3D positions projected on the best fitting plane.
 * @sa svtkTextureMapToPlane svtkResampleToImage
 */

#ifndef svtkScalarsToTextureFilter_h
#define svtkScalarsToTextureFilter_h

#include "svtkFiltersTextureModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"
#include "svtkSmartPointer.h" // For smart pointer

class svtkImageData;
class svtkPolyData;
class svtkScalarsToColors;

class SVTKFILTERSTEXTURE_EXPORT svtkScalarsToTextureFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkScalarsToTextureFilter* New();
  svtkTypeMacro(svtkScalarsToTextureFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get a color transfer function.
   * This transfer function will be used to determine the pixel colors of the texture.
   * If not specified, the filter use a default one (blue/white/red) based on the range of the
   * input array.
   */
  void SetTransferFunction(svtkScalarsToColors* stc);
  svtkScalarsToColors* GetTransferFunction();
  //@}

  //@{
  /**
   * Specify if a new point array containing RGBA values have to be computed by the specified
   * color transfer function.
   */
  svtkGetMacro(UseTransferFunction, bool);
  svtkSetMacro(UseTransferFunction, bool);
  svtkBooleanMacro(UseTransferFunction, bool);
  //@}

  //@{
  /**
   * Get/Set the width and height of the generated texture.
   * Default is 128x128. The width and height must be greater than 1.
   */
  svtkSetVector2Macro(TextureDimensions, int);
  svtkGetVector2Macro(TextureDimensions, int);
  //@}

protected:
  svtkScalarsToTextureFilter();
  ~svtkScalarsToTextureFilter() override = default;

  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  void operator=(const svtkScalarsToTextureFilter&) = delete;
  svtkScalarsToTextureFilter(const svtkScalarsToTextureFilter&) = delete;

  svtkSmartPointer<svtkScalarsToColors> TransferFunction;
  int TextureDimensions[2];
  bool UseTransferFunction = true;
};
#endif
