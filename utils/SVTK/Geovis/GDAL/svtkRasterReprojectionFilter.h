/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRasterReprojectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkRasterReprojectionFilter
 * @brief Transform a SVTK image data to a different projection.
 *
 * Applies map reprojection to svtkUniformGrid or svtkImageData.
 * Internally uses GDAL/Proj4 for the reprojection calculations.
 */

#ifndef svtkRasterReprojectionFilter_h
#define svtkRasterReprojectionFilter_h

#include "svtkGeovisGDALModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKGEOVISGDAL_EXPORT svtkRasterReprojectionFilter : public svtkImageAlgorithm
{
public:
  static svtkRasterReprojectionFilter* New();
  svtkTypeMacro(svtkRasterReprojectionFilter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the map-projection string for the input image data.
   * This should *only* be used for nonstandard image inputs,
   * when the MAP_PROJECTION is not embedded as field data.
   * Can be specified using any string formats supported by GDAL,
   * such as "well known text" (WKT) formats (GEOGS[]),
   * or shorter "user string" formats, such as EPSG:3857.
   */
  svtkSetStringMacro(InputProjection);
  svtkGetStringMacro(InputProjection);
  //@}

  //@{
  /**
   * Set the map-projection string for the output image data.
   */
  svtkSetStringMacro(OutputProjection);
  svtkGetStringMacro(OutputProjection);
  //@}

  //@{
  /**
   * Set the width and height of the output image.
   * It is recommended to leave this variable unset, in which case,
   * the filter will use the GDAL suggested dimensions to construct
   * the output image. This method can be used to override this, and
   * impose specific output image dimensions.
   */
  svtkSetVector2Macro(OutputDimensions, int);
  svtkGetVector2Macro(OutputDimensions, int);
  //@}

  //@{
  /**
   * The data value to use internally to represent blank points in GDAL
   * datasets. By default, this will be set to the minimum value for the input
   * data type.
   */
  svtkSetMacro(NoDataValue, double);
  svtkGetMacro(NoDataValue, double);
  //@}

  //@{
  /**
   * Set the maximum error, measured in input pixels, that is allowed
   * in approximating the GDAL reprojection transformation.
   * The default is 0.0, for exact calculations.
   */
  svtkSetClampMacro(MaxError, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaxError, double);
  //@}

  //@{
  /**
   * Set the pixel resampling algorithm. Choices range between 0 and 6:
   * 0 = Nearest Neighbor (default)
   * 1 = Bilinear
   * 2 = Cubic
   * 3 = CubicSpline
   * 4 = Lanczos
   * 5 = Average
   * 6 = Mode
   */
  svtkSetClampMacro(ResamplingAlgorithm, int, 0, 6);
  //@}

protected:
  svtkRasterReprojectionFilter();
  ~svtkRasterReprojectionFilter() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  char* InputProjection;
  int FlipAxis[3];
  char* OutputProjection;
  int OutputDimensions[2];
  double NoDataValue;
  double MaxError;
  int ResamplingAlgorithm;

  class svtkRasterReprojectionFilterInternal;
  svtkRasterReprojectionFilterInternal* Internal;

private:
  svtkRasterReprojectionFilter(const svtkRasterReprojectionFilter&) = delete;
  void operator=(const svtkRasterReprojectionFilter&) = delete;
};

#endif // svtkRasterReprojectionFilter_h
