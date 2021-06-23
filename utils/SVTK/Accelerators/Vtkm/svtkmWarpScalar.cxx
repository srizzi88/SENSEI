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
#include "svtkmWarpScalar.h"
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

#include <svtkm/filter/WarpScalar.h>

svtkStandardNewMacro(svtkmWarpScalar);

//------------------------------------------------------------------------------
svtkmWarpScalar::svtkmWarpScalar()
  : svtkWarpScalar()
{
}

//------------------------------------------------------------------------------
svtkmWarpScalar::~svtkmWarpScalar() {}

//------------------------------------------------------------------------------
int svtkmWarpScalar::RequestData(svtkInformation* svtkNotUsed(request),
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

  output->CopyStructure(input);

  // Get the scalar field info
  svtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  int inScalarsAssociation = this->GetInputArrayAssociation(0, inputVector);
  // Get the normal field info
  svtkDataArray* inNormals = input->GetPointData()->GetNormals();
  svtkPoints* inPts = input->GetPoints();

  // InScalars is not used when XYPlane is on
  if (!inPts || (!inScalars && !this->XYPlane))
  {
    svtkDebugMacro(<< "No data to warp");
    return 1;
  }

  try
  {
    svtkm::cont::DataSet in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);
    if (inScalars)
    {
      auto scalarFactor = tosvtkm::Convert(inScalars, inScalarsAssociation);
      in.AddField(scalarFactor);
    }
    svtkm::Id numberOfPoints = in.GetCoordinateSystem().GetData().GetNumberOfValues();

    // ScaleFactor in svtk is the scalarAmount in svtk-m.
    svtkm::filter::WarpScalar warpScalar(this->ScaleFactor);
    warpScalar.SetUseCoordinateSystemAsField(true);

    // Get/generate the normal field
    if (inNormals && !this->UseNormal)
    { // DataNormal
      auto inNormalsField = tosvtkm::Convert(inNormals, svtkDataObject::FIELD_ASSOCIATION_POINTS);
      in.AddField(inNormalsField);
      warpScalar.SetNormalField(inNormals->GetName());
    }
    else if (this->XYPlane)
    {
      using vecType = svtkm::Vec<svtkm::FloatDefault, 3>;
      vecType normal = svtkm::make_Vec<svtkm::FloatDefault>(0.0, 0.0, 1.0);
      svtkm::cont::ArrayHandleConstant<vecType> vectorAH =
        svtkm::cont::make_ArrayHandleConstant(normal, numberOfPoints);
      svtkm::cont::DataSetFieldAdd::AddPointField(in, "zNormal", vectorAH);
      warpScalar.SetNormalField("zNormal");
    }
    else
    {
      using vecType = svtkm::Vec<svtkm::FloatDefault, 3>;
      vecType normal =
        svtkm::make_Vec<svtkm::FloatDefault>(this->Normal[0], this->Normal[1], this->Normal[2]);
      svtkm::cont::ArrayHandleConstant<vecType> vectorAH =
        svtkm::cont::make_ArrayHandleConstant(normal, numberOfPoints);
      svtkm::cont::DataSetFieldAdd::AddPointField(in, "instanceNormal", vectorAH);
      warpScalar.SetNormalField("instanceNormal");
    }

    if (this->XYPlane)
    { // Just use the z value to warp the surface. Ignore the input scalars.
      std::vector<svtkm::FloatDefault> zValues;
      zValues.reserve(static_cast<size_t>(input->GetNumberOfPoints()));
      for (svtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
      {
        zValues.push_back(input->GetPoints()->GetPoint(i)[2]);
      }
      svtkm::cont::DataSetFieldAdd::AddPointField(in, "scalarfactor", zValues);
      warpScalar.SetScalarFactorField("scalarfactor");
    }
    else
    {
      warpScalar.SetScalarFactorField(std::string(inScalars->GetName()));
    }

    svtkmInputFilterPolicy policy;
    auto result = warpScalar.Execute(in, policy);
    svtkDataArray* warpScalarResult =
      fromsvtkm::Convert(result.GetField("warpscalar", svtkm::cont::Field::Association::POINTS));
    svtkPoints* newPts = svtkPoints::New();
    // Update points
    newPts->SetNumberOfPoints(warpScalarResult->GetNumberOfTuples());
    newPts->SetData(warpScalarResult);
    output->SetPoints(newPts);
    newPts->Delete();
    warpScalarResult->FastDelete();
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
void svtkmWarpScalar::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
