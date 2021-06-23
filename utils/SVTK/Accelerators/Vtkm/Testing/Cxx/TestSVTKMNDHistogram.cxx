/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVTKMClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkmNDHistogram.h"

#include "svtkActor.h"
#include "svtkArrayData.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDelaunay3D.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkImageToPoints.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkSparseArray.h"
#include "svtkTable.h"

namespace
{
static const std::vector<std::string> arrayNames = { "temperature0", "temperature1", "temperature2",
  "temperature3" };
static const std::vector<std::vector<size_t> > resultBins = {
  { 0, 0, 1, 1, 2, 2, 3, 3 },
  { 0, 1, 1, 2, 2, 3, 3, 4 },
  { 0, 1, 2, 2, 3, 4, 4, 5 },
  { 0, 1, 2, 3, 3, 4, 5, 6 },
};
static const std::vector<size_t> resultFrequency = { 2, 1, 1, 1, 1, 1, 1, 2 };
static const int nData = 10;
static const std::vector<size_t> bins = { 4, 5, 6, 7 };
void AddArrayToSVTKData(
  std::string scalarName, svtkDataSetAttributes* pd, double* data, svtkIdType size)
{
  svtkNew<svtkDoubleArray> scalars;
  scalars->SetArray(data, size, 1);
  scalars->SetName(scalarName.c_str());
  pd->AddArray(scalars);
}

void MakeTestDataset(svtkDataSet* dataset)
{
  static double T0[nData], T1[nData], T2[nData], T3[nData];
  for (int i = 0; i < nData; i++)
  {
    T0[i] = i * 1.0;
    T1[i] = i * 2.0;
    T2[i] = i * 3.0;
    T3[i] = i * 4.0;
  }

  svtkPointData* pd = dataset->GetPointData();
  AddArrayToSVTKData(arrayNames[0], pd, T0, static_cast<svtkIdType>(nData));
  AddArrayToSVTKData(arrayNames[1], pd, T1, static_cast<svtkIdType>(nData));
  AddArrayToSVTKData(arrayNames[2], pd, T2, static_cast<svtkIdType>(nData));
  AddArrayToSVTKData(arrayNames[3], pd, T3, static_cast<svtkIdType>(nData));
}
}

int TestSVTKMNDHistogram(int, char*[])
{
  svtkNew<svtkPolyData> ds;
  MakeTestDataset(ds);

  svtkNew<svtkmNDHistogram> filter;
  filter->SetInputData(ds);
  size_t index = 0;
  for (const auto& an : arrayNames)
  {
    filter->AddFieldAndBin(an, bins[index++]);
  }
  filter->Update();
  svtkArrayData* arrayData = filter->GetOutput();

  assert(arrayData != nullptr);
  // Valid the data range and bin delta
  for (svtkIdType i = 0; i < 4; i++)
  {
    // Validate the delta and range
    auto range = filter->GetDataRange(i);
    double delta = filter->GetBinDelta(i);
    if (range.first != 0.0 || range.second != (i + 1) * 9)
    {
      std::cout << "array index=" << i << " does not have right range" << std::endl;
      return 1;
    }
    if (delta != ((range.second - range.first) / bins[i]))
    {
      std::cout << "array index" << i << " does not have right delta" << std::endl;
      return 1;
    }
  }
  svtkSparseArray<double>* sa = static_cast<svtkSparseArray<double>*>(arrayData->GetArray(0));
  svtkArrayCoordinates coordinates;
  const svtkIdType dimensions = sa->GetDimensions();     // 4
  const svtkIdType non_null_size = sa->GetNonNullSize(); // 8
  for (svtkIdType n = 0; n != non_null_size; ++n)
  {
    sa->GetCoordinatesN(n, coordinates);
    for (svtkIdType d = 0; d != dimensions; ++d)
    {
      assert(coordinates[d] == static_cast<svtkIdType>(resultBins[d][n]));
      if (coordinates[d] != static_cast<svtkIdType>(resultBins[d][n]))
      {
        std::cout << "value does not match at index " << n << " dimension " << d << std::endl;
      }
    }
    assert(resultFrequency[n] == sa->GetValue(coordinates));
  }
  return 0;
}
