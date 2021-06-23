/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmPointTransform.h"

#include "svtkCellData.h"
#include "svtkHomogeneousTransform.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkm/cont/Error.h"
#include "svtkm/filter/PointTransform.h"
#include "svtkm/filter/PointTransform.hxx"

#include "svtkmFilterPolicy.h"

svtkStandardNewMacro(svtkmPointTransform);
svtkCxxSetObjectMacro(svtkmPointTransform, Transform, svtkHomogeneousTransform);

//------------------------------------------------------------------------------
svtkmPointTransform::svtkmPointTransform()
{
  this->Transform = nullptr;
}

//------------------------------------------------------------------------------
svtkmPointTransform::~svtkmPointTransform()
{
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
int svtkmPointTransform::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int svtkmPointTransform::RequestDataObject(
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
int svtkmPointTransform::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkPointSet> input = svtkPointSet::GetData(inputVector[0]);
  svtkSmartPointer<svtkPointSet> output = svtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting rectilinear grid
    svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      svtkNew<svtkRectilinearGridToPointSet> rectToPoints;
      rectToPoints->SetInputData(inRect);
      rectToPoints->Update();
      input = rectToPoints->GetOutput();
    }
  }
  if (!input)
  {
    svtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  output->CopyStructure(input);

  svtkPoints* inPts = input->GetPoints();

  if (!inPts || !this->Transform)
  {
    svtkDebugMacro(<< "Miss input points or transform matrix");
    return 0;
  }

  try
  {
    svtkm::cont::DataSet in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);
    svtkMatrix4x4* matrix = this->Transform->GetMatrix();
    svtkm::Matrix<svtkm::FloatDefault, 4, 4> svtkmMatrix;
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        svtkmMatrix[i][j] = static_cast<svtkm::FloatDefault>(matrix->GetElement(i, j));
      }
    }

    svtkm::filter::PointTransform pointTransform;
    pointTransform.SetUseCoordinateSystemAsField(true);
    pointTransform.SetTransform(svtkmMatrix);

    svtkmInputFilterPolicy policy;
    auto result = pointTransform.Execute(in, policy);
    svtkDataArray* pointTransformResult =
      fromsvtkm::Convert(result.GetField("transform", svtkm::cont::Field::Association::POINTS));
    svtkPoints* newPts = svtkPoints::New();
    // Update points
    newPts->SetNumberOfPoints(pointTransformResult->GetNumberOfTuples());
    newPts->SetData(pointTransformResult);
    output->SetPoints(newPts);
    newPts->FastDelete();
    pointTransformResult->FastDelete();
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
void svtkmPointTransform::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Transform: " << this->Transform << "\n";
}
