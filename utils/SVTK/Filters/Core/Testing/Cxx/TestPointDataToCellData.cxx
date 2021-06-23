/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellData.h>
#include <svtkDataArray.h>
#include <svtkDataSet.h>
#include <svtkDataSetTriangleFilter.h>
#include <svtkDoubleArray.h>
#include <svtkImageData.h>
#include <svtkPointData.h>
#include <svtkPointDataToCellData.h>
#include <svtkRTAnalyticSource.h>
#include <svtkTestUtilities.h>
#include <svtkThreshold.h>
#include <svtkUnstructuredGrid.h>

int TestPointDataToCellData(int, char*[])
{
  char const name[] = "RTData";
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  wavelet->SetCenter(0, 0, 0);
  wavelet->SetMaximum(255);
  wavelet->SetStandardDeviation(.5);
  wavelet->SetXFreq(60);
  wavelet->SetYFreq(30);
  wavelet->SetZFreq(40);
  wavelet->SetXMag(10);
  wavelet->SetYMag(18);
  wavelet->SetZMag(5);
  wavelet->SetSubsampleRate(1);
  wavelet->Update();

  svtkNew<svtkDoubleArray> dist;
  dist->SetNumberOfComponents(1);
  dist->SetName("Dist");

  svtkImageData* original = wavelet->GetOutput();
  for (svtkIdType i = 0; i < original->GetNumberOfPoints(); ++i)
  {
    double p[3];
    original->GetPoint(i, p);
    dist->InsertNextValue(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
  }
  original->GetPointData()->AddArray(dist);

  svtkNew<svtkPointDataToCellData> p2c;
  p2c->SetInputData(original);
  p2c->SetProcessAllArrays(false);
  p2c->AddPointDataArray(name);
  p2c->PassPointDataOff();
  p2c->Update();

  // test if selective CellDataToPointData operates on the correct
  int outNumPArrays = p2c->GetOutput()->GetPointData()->GetNumberOfArrays(); // should be 0
  int outNumCArrays = p2c->GetOutput()->GetCellData()->GetNumberOfArrays();  // should be 1
  std::string cArrayName = p2c->GetOutput()->GetCellData()->GetArrayName(0); // should be RTData

  if (outNumPArrays != 0)
  {
    std::cerr << "Wrong number of PointData arrays." << std::endl;
    return EXIT_FAILURE;
  }

  if (outNumCArrays != 1)
  {
    std::cerr << "Wrong number of CellData arrays." << std::endl;
    return EXIT_FAILURE;
  }

  if (cArrayName != name)
  {
    std::cerr << "Array name not matching original name." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
