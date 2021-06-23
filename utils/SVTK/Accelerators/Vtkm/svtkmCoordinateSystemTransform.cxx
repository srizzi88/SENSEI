//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "svtkmCoordinateSystemTransform.h"
#include "svtkmConfig.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/CoordinateSystemTransform.h>

svtkStandardNewMacro(svtkmCoordinateSystemTransform);

//------------------------------------------------------------------------------
svtkmCoordinateSystemTransform::svtkmCoordinateSystemTransform()
{
  this->TransformType = TransformTypes::None;
}

//------------------------------------------------------------------------------
svtkmCoordinateSystemTransform::~svtkmCoordinateSystemTransform() {}

//------------------------------------------------------------------------------
void svtkmCoordinateSystemTransform::SetCartesianToCylindrical()
{
  this->TransformType = TransformTypes::CarToCyl;
}

//------------------------------------------------------------------------------
void svtkmCoordinateSystemTransform::SetCylindricalToCartesian()
{
  this->TransformType = TransformTypes::CylToCar;
}

//------------------------------------------------------------------------------
void svtkmCoordinateSystemTransform::SetCartesianToSpherical()
{
  this->TransformType = TransformTypes::CarToSph;
}

//------------------------------------------------------------------------------
void svtkmCoordinateSystemTransform::SetSphericalToCartesian()
{
  this->TransformType = TransformTypes::SphToCar;
}

//------------------------------------------------------------------------------
int svtkmCoordinateSystemTransform::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int svtkmCoordinateSystemTransform::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
  svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    svtkStructuredGrid* output = svtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      svtkNew<svtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//------------------------------------------------------------------------------
int svtkmCoordinateSystemTransform::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkPointSet> input = svtkPointSet::GetData(inputVector[0]);
  svtkPointSet* output = svtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      svtkNew<svtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      svtkNew<svtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    svtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  output->CopyStructure(input);

  svtkPoints* inPts = input->GetPoints();

  if (!inPts || this->TransformType == TransformTypes::None)
  {
    svtkErrorMacro(<< "Miss input points or transform type has not been specified");
    return 0;
  }

  try
  {
    svtkm::cont::DataSet in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::Points);
    svtkmInputFilterPolicy policy;
    svtkDataArray* transformResult;
    if (this->TransformType == TransformTypes::CarToCyl ||
      this->TransformType == TransformTypes::CylToCar)
    { // Cylindrical coordinate transform
      svtkm::filter::CylindricalCoordinateTransform cylindricalCT;
      cylindricalCT.SetUseCoordinateSystemAsField(true);
      (this->TransformType == TransformTypes::CarToCyl) ? cylindricalCT.SetCartesianToCylindrical()
                                                        : cylindricalCT.SetCylindricalToCartesian();
      auto result = cylindricalCT.Execute(in, policy);
      transformResult = fromsvtkm::Convert(result.GetField(
        "cylindricalCoordinateSystemTransform", svtkm::cont::Field::Association::POINTS));
    }
    else
    { // Spherical coordinate system
      svtkm::filter::SphericalCoordinateTransform sphericalCT;
      sphericalCT.SetUseCoordinateSystemAsField(true);
      (this->TransformType == TransformTypes::CarToSph) ? sphericalCT.SetCartesianToSpherical()
                                                        : sphericalCT.SetSphericalToCartesian();
      auto result = sphericalCT.Execute(in, policy);
      transformResult = fromsvtkm::Convert(result.GetField(
        "sphericalCoordinateSystemTransform", svtkm::cont::Field::Association::POINTS));
    }
    svtkPoints* newPts = svtkPoints::New();
    // Update points
    newPts->SetNumberOfPoints(transformResult->GetNumberOfTuples());
    newPts->SetData(transformResult);
    output->SetPoints(newPts);
    newPts->Delete();
    transformResult->FastDelete();
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
  }

  // Update ourselves and release memory
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//------------------------------------------------------------------------------
void svtkmCoordinateSystemTransform::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
