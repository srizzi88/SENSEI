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

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkmProbe.h"

namespace
{
static int inputDim = 9;
static int sourceDim = 4;
void populatePointAndCellArray(svtkFloatArray* pointArray, svtkFloatArray* cellArray)
{
  pointArray->SetNumberOfValues(sourceDim * sourceDim);
  pointArray->SetName("pointdata");
  for (svtkIdType i = 0; i < static_cast<svtkIdType>(sourceDim * sourceDim); i++)
  {
    pointArray->SetValue(i, 0.3f * i);
  }
  cellArray->SetName("celldata");
  cellArray->SetNumberOfValues((sourceDim - 1) * (sourceDim - 1));
  for (svtkIdType i = 0; i < static_cast<svtkIdType>((sourceDim - 1) * (sourceDim - 1)); i++)
  {
    cellArray->SetValue(i, 0.7f * i);
  }
}

const std::vector<float>& GetExpectedPointData()
{
  static std::vector<float> expected = {
    1.05f, 1.155f, 1.26f, 1.365f, 1.47f, 1.575f, 1.68f, 0.0f, 0.0f, 1.47f, 1.575f, 1.68f,  //
    1.785f, 1.89f, 1.995f, 2.1f, 0.0f, 0.0f, 1.89f, 1.995f, 2.1f, 2.205f, 2.31f, 2.415f,   //
    2.52f, 0.0f, 0.0f, 2.31f, 2.415f, 2.52f, 2.625f, 2.73f, 2.835f, 2.94f, 0.0f, 0.0f,     //
    2.73f, 2.835f, 2.94f, 3.045f, 3.15f, 3.255f, 3.36f, 0.0f, 0.0f, 3.15f, 3.255f, 3.36f,  //
    3.465f, 3.57f, 3.675f, 3.78f, 0.0f, 0.0f, 3.57f, 3.675f, 3.78f, 3.885f, 3.99f, 4.095f, //
    4.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,                //
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f                                   //
  };
  return expected;
}

const std::vector<float>& GetExpectedCellData()
{
  static std::vector<float> expected = {
    0.0f, 0.7f, 0.7f, 0.7f, 1.4f, 1.4f, 1.4f, 0.0f, 0.0f, 2.1f, 2.8f, 2.8f, 2.8f, 3.5f, //
    3.5f, 3.5f, 0.0f, 0.0f, 2.1f, 2.8f, 2.8f, 2.8f, 3.5f, 3.5f, 3.5f, 0.0f, 0.0f, 2.1f, //
    2.8f, 2.8f, 2.8f, 3.5f, 3.5f, 3.5f, 0.0f, 0.0f, 4.2f, 4.9f, 4.9f, 4.9f, 5.6f, 5.6f, //
    5.6f, 0.0f, 0.0f, 4.2f, 4.9f, 4.9f, 4.9f, 5.6f, 5.6f, 5.6f, 0.0f, 0.0f, 4.2f, 4.9f, //
    4.9f, 4.9f, 5.6f, 5.6f, 5.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, //
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f                    //
  };
  return expected;
}

const std::vector<size_t>& GetExpectedHiddenPoints()
{
  static std::vector<size_t> expected = {
    0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, //
    2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, //
    2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, //
    0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, //
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2              //
  };
  return expected;
}

const std::vector<size_t>& GetExpectedHiddenCells()
{
  static std::vector<size_t> expected = {
    0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, //
    0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, //
    0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, //
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2  //
  };
  return expected;
}

template <typename T>
void TestResultArray(svtkDataArray* result, const std::vector<T>& expected)
{
  if (result->GetNumberOfValues() != static_cast<svtkIdType>(expected.size()))
  {
    std::cout << "Array " << result->GetName() << " has wrong size" << std::endl;
  }
  assert(result->GetNumberOfValues() == static_cast<svtkIdType>(expected.size()));

  for (svtkIdType i = 0; i < result->GetNumberOfValues(); ++i)
  {
    if ((result->GetComponent(0, i) - expected[static_cast<size_t>(i)]) > 1e-5)
    {
      std::cout << "Array " << result->GetName() << " has wrong value"
                << " at index " << i << ". result value=" << result->GetComponent(0, i)
                << " expected value=" << expected[static_cast<size_t>(i)] << std::endl;
    }
    assert((result->GetComponent(0, i) - expected[static_cast<size_t>(i)]) < 1e-5);
  }
}

} //  Anonymous namespace

int TestSVTKMProbe(int, char*[])
{
  svtkNew<svtkImageData> input;
  input->SetOrigin(0.7, 0.7, 0.0);
  input->SetSpacing(0.35, 0.35, 1.0);
  input->SetExtent(0, inputDim - 1, 0, inputDim - 1, 0, 0);

  svtkNew<svtkImageData> source;
  source->SetOrigin(0.0, 0.0, 0.0);
  source->SetSpacing(1.0, 1.0, 1.0);
  source->SetExtent(0, sourceDim - 1, 0, sourceDim - 1, 0, 0);

  svtkNew<svtkFloatArray> pointArray, cellArray;
  populatePointAndCellArray(pointArray, cellArray);
  source->GetPointData()->AddArray(pointArray);
  source->GetCellData()->AddArray(cellArray);

  svtkNew<svtkmProbe> probe;
  probe->SetValidPointMaskArrayName("validPoint");
  probe->SetValidCellMaskArrayName("validCell");
  probe->SetInputData(input);
  probe->SetSourceData(source);
  probe->Update();

  svtkDataSet* result = probe->GetOutput();
  TestResultArray(result->GetPointData()->GetArray(pointArray->GetName()), GetExpectedPointData());
  TestResultArray(result->GetCellData()->GetArray(cellArray->GetName()), GetExpectedCellData());
  TestResultArray(result->GetPointData()->GetArray("validPoint"), GetExpectedHiddenPoints());
  TestResultArray(result->GetCellData()->GetArray("validCell"), GetExpectedHiddenCells());
  return 0;
}
