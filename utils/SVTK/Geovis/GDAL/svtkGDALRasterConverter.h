/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGDALRasterConverter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkGDALRasterConverter
 * @brief Convert between SVTK image representation and GDAL datasets
 *
 * svtkGDALRasterConverter is an internal implementation class used to convert
 * between SVTK and GDAL data formats.
 *
 * @sa svtkRasterReprojectionFilter
 */

#ifndef svtkGDALRasterConverter_h
#define svtkGDALRasterConverter_h

#include "svtkGeovisGDALModule.h" // For export macro
#include "svtkObject.h"

// Forward declarations
class GDALDataset;
class svtkImageData;
class svtkUniformGrid;

class SVTKGEOVISGDAL_EXPORT svtkGDALRasterConverter : public svtkObject
{
public:
  static svtkGDALRasterConverter* New();
  svtkTypeMacro(svtkGDALRasterConverter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * No-data value for pixels in the source image
   * Default is NaN (not used).
   */
  svtkSetMacro(NoDataValue, double);
  svtkGetMacro(NoDataValue, double);
  //@}

  /**
   * Create GDAL dataset in memory.
   * This dataset must be released by the calling code,
   * using GDALClose().
   */
  GDALDataset* CreateGDALDataset(int xDim, int yDim, int svtkDataType, int numberOfBands);

  /**
   * Create GDALDataset to match svtkImageData.
   * This dataset must be released by the calling code,
   * using GDALClose().
   */
  GDALDataset* CreateGDALDataset(svtkImageData* data, const char* mapProjection, int flipAxis[3]);

  /**
   * Copies color interpretation and color tables
   */
  void CopyBandInfo(GDALDataset* src, GDALDataset* dest);

  /**
   * Create svtkUniformGrid to match GDALDataset.
   * The calling code must call the Delete() method
   * to release the returned instance.
   */
  svtkUniformGrid* CreateSVTKUniformGrid(GDALDataset* input);

  /**
   * Set projection on GDAL dataset, using any projection string
   * recognized by GDAL.
   */
  void SetGDALProjection(GDALDataset* dataset, const char* projectionString);

  /**
   * Set geo-transform on GDAL dataset.
   */
  void SetGDALGeoTransform(
    GDALDataset* dataset, double origin[2], double spacing[2], int flipAxis[2]);

  /**
   * Copies NoDataValue info from 1st to 2nd dataset
   */
  void CopyNoDataValues(GDALDataset* src, GDALDataset* dest);

  /**
   * Write GDALDataset to tiff file
   */
  void WriteTifFile(GDALDataset* dataset, const char* filename);

  /**
   * Traverse values in specified band to find min/max.
   * Note that the bandId starts at 1, not zero.
   * Returns boolean indicating success.
   */
  bool FindDataRange(GDALDataset* dataset, int bandId, double* minValue, double* maxValue);

protected:
  svtkGDALRasterConverter();
  ~svtkGDALRasterConverter() override;

  double NoDataValue;

  /**
   * Copies svtkImageData contents to GDALDataset
   * GDALDataset must be initialized to same dimensions as svtk image.
   */
  bool CopyToGDAL(svtkImageData* input, GDALDataset* output, int flipAxis[3]);

  class svtkGDALRasterConverterInternal;
  svtkGDALRasterConverterInternal* Internal;

private:
  svtkGDALRasterConverter(const svtkGDALRasterConverter&) = delete;
  void operator=(const svtkGDALRasterConverter&) = delete;
};

#endif // svtkGDALRasterConverter_h
