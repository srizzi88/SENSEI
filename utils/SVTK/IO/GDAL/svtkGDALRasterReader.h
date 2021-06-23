/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGDALRasterReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGDALRasterReader
 * @brief   Read raster file formats using GDAL.
 *
 * svtkGDALRasterReader is a source object that reads raster files and
 * uses GDAL as the underlying library for the task. GDAL library is
 * required for this reader. The output of the reader is a
 * svtkUniformGrid (svtkImageData with blanking) with cell data.
 * The reader currently supports only north up images. Flips along
 * X or Y direction are also supported. Arbitrary affine geotransforms or
 * GCPs are not supported. See GDAL Data Model for more information
 * https://www.gdal.org/gdal_datamodel.html
 *
 *
 *
 * @sa
 * svtkUniformGrid, svtkImageData
 */

#ifndef svtkGDALRasterReader_h
#define svtkGDALRasterReader_h

#include <svtkIOGDALModule.h> // For export macro
#include <svtkImageReader2.h>

// C++ includes
#include <string> // string is required
#include <vector> // vector is required

class SVTKIOGDAL_EXPORT svtkGDALRasterReader : public svtkImageReader2
{
public:
  static svtkGDALRasterReader* New();
  svtkTypeMacro(svtkGDALRasterReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkGDALRasterReader();
  ~svtkGDALRasterReader() override;

  /**
   * Is this file supported
   */
  int CanReadFile(const char* fname) override;

  /**
   * Return proj4 spatial reference
   */
  const char* GetProjectionString() const;

  /**
   * Returns WKT spatial reference.
   */
  const char* GetProjectionWKT() const { return this->ProjectionWKT.c_str(); }

  /**
   * Return geo-referenced corner points (Upper left,
   * lower left, lower right, upper right)
   */
  const double* GetGeoCornerPoints();

  /**
   * Get/Set if bands are collated in one scalar array.
   * Currently we collate RGB, RGBA, gray alpha and palette.
   * The default is true.
   */
  svtkSetMacro(CollateBands, bool);
  svtkGetMacro(CollateBands, bool);
  svtkBooleanMacro(CollateBands, bool);

  //@{
  /**
   * Set desired width and height of the image
   */
  svtkSetVector2Macro(TargetDimensions, int);
  svtkGetVector2Macro(TargetDimensions, int);
  //@}

  //@{
  /**
   * Get raster width and height in number of pixels (cells)
   */
  int* GetRasterDimensions();
  //@}

  /**
   * Return metadata as reported by GDAL
   */
  const std::vector<std::string>& GetMetaData();

  /**
   * Return the invalid value for a pixel (for blanking purposes) in
   * a specified raster band. Note bandIndex is a 0 based index while
   * GDAL bands are 1 based indexes. hasNoData indicates if there is a NoData
   * value associated with this band.
   */
  double GetInvalidValue(size_t bandIndex = 0, int* hasNoData = nullptr);

  /**
   * Return domain metadata
   */
  std::vector<std::string> GetDomainMetaData(const std::string& domain);

  //@{
  /**
   * Return driver name which was used to read the current data
   */
  const std::string& GetDriverShortName();
  const std::string& GetDriverLongName();
  //@}

  /**
   * Return the number of cells that are not set to GDAL NODATA
   */
  svtkIdType GetNumberOfCells();

  //@{
  /**
   * The following methods allow selective reading of bands.
   * By default, ALL bands are read.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  //@}

protected:
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

protected:
  int TargetDimensions[2];
  std::string Projection;
  std::string ProjectionWKT;
  std::string DomainMetaData;
  std::string DriverShortName;
  std::string DriverLongName;
  std::vector<std::string> Domains;
  std::vector<std::string> MetaData;
  bool CollateBands;

  class svtkGDALRasterReaderInternal;
  svtkGDALRasterReaderInternal* Impl;

private:
  svtkGDALRasterReader(const svtkGDALRasterReader&) = delete;
  void operator=(const svtkGDALRasterReader&) = delete;
};

#endif // svtkGDALRasterReader_h
