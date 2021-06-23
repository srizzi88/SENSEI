/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRasterReprojectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRasterReprojectionFilter.h"

// SVTK includes
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkGDAL.h"
#include "svtkGDALRasterConverter.h"
#include "svtkGDALRasterReprojection.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGrid.h"

// GDAL includes
#include <gdal_priv.h>

// STL includes
#include <algorithm>

svtkStandardNewMacro(svtkRasterReprojectionFilter);

//----------------------------------------------------------------------------
class svtkRasterReprojectionFilter::svtkRasterReprojectionFilterInternal
{
public:
  svtkRasterReprojectionFilterInternal();
  ~svtkRasterReprojectionFilterInternal();

  svtkGDALRasterConverter* GDALConverter;
  svtkGDALRasterReprojection* GDALReprojection;

  // Data saved during RequestInformation()
  int InputImageExtent[6];
  double OutputImageGeoTransform[6];
};

//----------------------------------------------------------------------------
svtkRasterReprojectionFilter::svtkRasterReprojectionFilterInternal::
  svtkRasterReprojectionFilterInternal()
{
  this->GDALConverter = svtkGDALRasterConverter::New();
  this->GDALReprojection = svtkGDALRasterReprojection::New();
  std::fill(this->InputImageExtent, this->InputImageExtent + 6, 0);
  std::fill(this->OutputImageGeoTransform, this->OutputImageGeoTransform + 6, 0.0);
}

//----------------------------------------------------------------------------
svtkRasterReprojectionFilter::svtkRasterReprojectionFilterInternal::
  ~svtkRasterReprojectionFilterInternal()
{
  this->GDALConverter->Delete();
  this->GDALReprojection->Delete();
}

//----------------------------------------------------------------------------
svtkRasterReprojectionFilter::svtkRasterReprojectionFilter()
{
  this->Internal = new svtkRasterReprojectionFilterInternal;
  this->InputProjection = nullptr;
  this->FlipAxis[0] = this->FlipAxis[1] = this->FlipAxis[2] = 0;
  this->OutputProjection = nullptr;
  this->OutputDimensions[0] = this->OutputDimensions[1] = 0;
  this->NoDataValue = svtkMath::Nan();
  this->MaxError = 0.0;
  this->ResamplingAlgorithm = 0;

  // Enable all the drivers.
  GDALAllRegister();
}

//----------------------------------------------------------------------------
svtkRasterReprojectionFilter::~svtkRasterReprojectionFilter()
{
  if (this->InputProjection)
  {
    delete[] this->InputProjection;
  }
  if (this->OutputProjection)
  {
    delete[] this->OutputProjection;
  }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkRasterReprojectionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputProjection: ";
  if (this->InputProjection)
  {
    os << this->InputProjection;
  }
  else
  {
    os << "(not specified)";
  }
  os << "\n";

  os << indent << "OutputProjection: ";
  if (this->OutputProjection)
  {
    os << this->OutputProjection;
  }
  else
  {
    os << "(not specified)";
  }
  os << "\n";

  os << indent << "OutputDimensions: " << OutputDimensions[0] << ", " << OutputDimensions[1] << "\n"
     << indent << "NoDataValue: " << this->NoDataValue << "\n"
     << indent << "MaxError: " << this->MaxError << "\n"
     << indent << "ResamplingAlgorithm: " << this->ResamplingAlgorithm << "\n"
     << indent << "FlipAxis: " << this->FlipAxis[0] << ", " << this->FlipAxis[1] << "\n"
     << std::endl;
}

//-----------------------------------------------------------------------------
int svtkRasterReprojectionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input image data
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataObject* inDataObject = inInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!inDataObject)
  {
    return 0;
  }

  svtkImageData* inImageData = svtkImageData::SafeDownCast(inDataObject);
  if (!inImageData)
  {
    return 0;
  }
  // std::cout << "RequestData() has image to reproject!" << std::endl;

  // Get the output image
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    return 0;
  }

  // Convert input image to GDALDataset
  GDALDataset* inputGDAL = this->Internal->GDALConverter->CreateGDALDataset(
    inImageData, this->InputProjection, this->FlipAxis);

  if (this->Debug)
  {
    std::string tifFileName = "inputGDAL.tif";
    this->Internal->GDALConverter->WriteTifFile(inputGDAL, tifFileName.c_str());
    std::cout << "Wrote " << tifFileName << std::endl;

    double minValue, maxValue;
    this->Internal->GDALConverter->FindDataRange(inputGDAL, 1, &minValue, &maxValue);
    std::cout << "Min: " << minValue << "  Max: " << maxValue << std::endl;
  }

  // Construct GDAL dataset for output image
  svtkDataArray* array = inImageData->GetCellData()->GetScalars();
  int svtkDataType = array->GetDataType();
  int rasterCount = array->GetNumberOfComponents();
  GDALDataset* outputGDAL = this->Internal->GDALConverter->CreateGDALDataset(
    this->OutputDimensions[0], this->OutputDimensions[1], svtkDataType, rasterCount);
  this->Internal->GDALConverter->CopyBandInfo(inputGDAL, outputGDAL);
  this->Internal->GDALConverter->SetGDALProjection(outputGDAL, this->OutputProjection);
  outputGDAL->SetGeoTransform(this->Internal->OutputImageGeoTransform);
  this->Internal->GDALConverter->CopyNoDataValues(inputGDAL, outputGDAL);

  // Apply the reprojection
  this->Internal->GDALReprojection->SetMaxError(this->MaxError);
  this->Internal->GDALReprojection->SetResamplingAlgorithm(this->ResamplingAlgorithm);
  this->Internal->GDALReprojection->Reproject(inputGDAL, outputGDAL);

  if (this->Debug)
  {
    std::string tifFileName = "reprojectGDAL.tif";
    this->Internal->GDALConverter->WriteTifFile(outputGDAL, tifFileName.c_str());
    std::cout << "Wrote " << tifFileName << std::endl;
    double minValue, maxValue;
    this->Internal->GDALConverter->FindDataRange(outputGDAL, 1, &minValue, &maxValue);
    std::cout << "Min: " << minValue << "  Max: " << maxValue << std::endl;
  }

  // Done with input GDAL dataset
  GDALClose(inputGDAL);

  // Convert output dataset to svtkUniformGrid
  svtkUniformGrid* reprojectedImage =
    this->Internal->GDALConverter->CreateSVTKUniformGrid(outputGDAL);

  // Done with output GDAL dataset
  GDALClose(outputGDAL);

  // Update pipeline output instance
  svtkUniformGrid* output = svtkUniformGrid::GetData(outInfo);
  output->ShallowCopy(reprojectedImage);

  reprojectedImage->Delete();
  return SVTK_OK;
}

//-----------------------------------------------------------------------------
int svtkRasterReprojectionFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // Set input extent to values saved in last RequestInformation() call
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), this->Internal->InputImageExtent, 6);
  return SVTK_OK;
}

//-----------------------------------------------------------------------------
int svtkRasterReprojectionFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) ||
    !inInfo->Has(svtkDataObject::SPACING()) || !inInfo->Has(svtkDataObject::ORIGIN()))
  {
    svtkErrorMacro("Input information missing");
    return SVTK_ERROR;
  }
  int* inputDataExtent = inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  std::copy(inputDataExtent, inputDataExtent + 6, this->Internal->InputImageExtent);

  double* inputOrigin = inInfo->Get(svtkDataObject::ORIGIN());
  double* inputSpacing = inInfo->Get(svtkDataObject::SPACING());
  // std::cout << "Whole extent: " << inputDataExtent[0]
  //           << ", " << inputDataExtent[1]
  //           << ", " << inputDataExtent[2]
  //           << ", " << inputDataExtent[3]
  //           << ", " << inputDataExtent[4]
  //           << ", " << inputDataExtent[5]
  //           << std::endl;
  // std::cout << "Input spacing: " << inputSpacing[0]
  //           << ", " << inputSpacing[1]
  //           << ", " << inputSpacing[2] << std::endl;
  // std::cout << "Input origin: " << inputOrigin[0]
  //           << ", " << inputOrigin[1]
  //           << ", " << inputOrigin[2] << std::endl;

  // InputProjection can be overridden, so only get from pipeline if needed
  if (!this->InputProjection)
  {
    if (!inInfo->Has(svtkGDAL::MAP_PROJECTION()))
    {
      svtkErrorMacro("No map-projection for input image");
      return SVTK_ERROR;
    }
    this->SetInputProjection(inInfo->Get(svtkGDAL::MAP_PROJECTION()));
  }
  if (!inInfo->Has(svtkGDAL::FLIP_AXIS()))
  {
    svtkErrorMacro("No flip information for GDAL raster input image");
    return SVTK_ERROR;
  }
  inInfo->Get(svtkGDAL::FLIP_AXIS(), this->FlipAxis);

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    svtkErrorMacro("Invalid output information object");
    return SVTK_ERROR;
  }

  // Validate current settings
  if (!this->OutputProjection)
  {
    svtkErrorMacro("No output projection specified");
    return SVTK_ERROR;
  }

  // Create GDALDataset to compute suggested output
  int xDim = inputDataExtent[1] - inputDataExtent[0] + 1;
  int yDim = inputDataExtent[3] - inputDataExtent[2] + 1;
  GDALDataset* gdalDataset =
    this->Internal->GDALConverter->CreateGDALDataset(xDim, yDim, SVTK_UNSIGNED_CHAR, 1);
  this->Internal->GDALConverter->SetGDALProjection(gdalDataset, this->InputProjection);
  this->Internal->GDALConverter->SetGDALGeoTransform(
    gdalDataset, inputOrigin, inputSpacing, this->FlipAxis);

  int nPixels = 0;
  int nLines = 0;
  this->Internal->GDALReprojection->SuggestOutputDimensions(gdalDataset, this->OutputProjection,
    this->Internal->OutputImageGeoTransform, &nPixels, &nLines);
  GDALClose(gdalDataset);

  if ((this->OutputDimensions[0] < 1) || (this->OutputDimensions[1] < 1))
  {
    this->OutputDimensions[0] = nPixels;
    this->OutputDimensions[1] = nLines;
  }

  // Set output info
  int outputDataExtent[6] = {};
  outputDataExtent[1] = this->OutputDimensions[0] - 1;
  outputDataExtent[3] = this->OutputDimensions[1] - 1;
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outputDataExtent, 6);

  double outputImageOrigin[3] = {};
  outputImageOrigin[0] = this->Internal->OutputImageGeoTransform[0];
  outputImageOrigin[1] = this->Internal->OutputImageGeoTransform[3];
  outputImageOrigin[2] = 1.0;
  outInfo->Set(svtkDataObject::SPACING(), outputImageOrigin, 3);

  double outputImageSpacing[3] = {};
  outputImageSpacing[0] = this->Internal->OutputImageGeoTransform[1];
  outputImageSpacing[1] = -this->Internal->OutputImageGeoTransform[5];
  outputImageSpacing[2] = 1.0;
  outInfo->Set(svtkDataObject::ORIGIN(), outputImageSpacing, 3);

  return SVTK_OK;
}

//-----------------------------------------------------------------------------
int svtkRasterReprojectionFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
    return SVTK_OK;
  }
  else
  {
    svtkErrorMacro("Input port: " << port << " is not a valid port");
    return SVTK_ERROR;
  }
  return SVTK_OK;
}

//-----------------------------------------------------------------------------
int svtkRasterReprojectionFilter::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUniformGrid");
    return SVTK_OK;
  }
  else
  {
    svtkErrorMacro("Output port: " << port << " is not a valid port");
    return SVTK_ERROR;
  }
}
