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
#include "svtkmWarpVector.h"
#include "svtkmConfig.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkm/cont/DataSetFieldAdd.h"
#include "svtkmFilterPolicy.h"

#include <svtkm/filter/WarpVector.h>

svtkStandardNewMacro(svtkmWarpVector);

//------------------------------------------------------------------------------
svtkmWarpVector::svtkmWarpVector()
  : svtkWarpVector()
{
}

//------------------------------------------------------------------------------
svtkmWarpVector::~svtkmWarpVector() {}

//------------------------------------------------------------------------------
int svtkmWarpVector::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkPointSet> input = svtkPointSet::GetData(inputVector[0]);
  svtkSmartPointer<svtkPointSet> output = svtkPointSet::GetData(outputVector);

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
  svtkIdType numPts = input->GetPoints()->GetNumberOfPoints();

  svtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);
  int vectorsAssociation = this->GetInputArrayAssociation(0, inputVector);

  if (!vectors || !numPts)
  {
    svtkDebugMacro(<< "no input data");
    return 1;
  }

  output->CopyStructure(input);

  try
  {
    svtkm::cont::DataSet in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);
    svtkm::cont::Field vectorField = tosvtkm::Convert(vectors, vectorsAssociation);
    in.AddField(vectorField);

    svtkmInputFilterPolicy policy;
    svtkm::filter::WarpVector warpVector(this->ScaleFactor);
    warpVector.SetUseCoordinateSystemAsField(true);
    warpVector.SetVectorField(vectorField.GetName(), vectorField.GetAssociation());
    auto result = warpVector.Execute(in, policy);

    svtkDataArray* warpVectorResult =
      fromsvtkm::Convert(result.GetField("warpvector", svtkm::cont::Field::Association::POINTS));
    svtkNew<svtkPoints> newPts;

    newPts->SetNumberOfPoints(warpVectorResult->GetNumberOfTuples());
    newPts->SetData(warpVectorResult);
    output->SetPoints(newPts);
    warpVectorResult->FastDelete();
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }

  // Update ourselves and release memory
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());
  return 1;
}

//------------------------------------------------------------------------------
void svtkmWarpVector::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
