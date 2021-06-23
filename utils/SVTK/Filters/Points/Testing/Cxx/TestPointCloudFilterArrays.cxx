/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointCloudFilterArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkRadiusOutlierRemoval.h>
#include <svtkSmartPointer.h>

#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>

#include <svtkCharArray.h>
#include <svtkIntArray.h>
#include <svtkLongArray.h>
#include <svtkShortArray.h>
#include <svtkUnsignedCharArray.h>
#include <svtkUnsignedIntArray.h>
#include <svtkUnsignedLongArray.h>
#include <svtkUnsignedShortArray.h>

template <typename T>
svtkSmartPointer<T> MakeArray(const std::string& name);

int TestPointCloudFilterArrays(int, char*[])
{
  const double pt1[3] = { 0, 0, 0 };
  const double pt2[3] = { 1, 0, 0 };
  const double pt3[3] = { 2, 0, 0 };

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);
  points->InsertNextPoint(pt3);

  // Generate arrays of integral types
  svtkSmartPointer<svtkUnsignedCharArray> uca = MakeArray<svtkUnsignedCharArray>("uca");
  svtkSmartPointer<svtkCharArray> ca = MakeArray<svtkCharArray>("ca");

  svtkSmartPointer<svtkUnsignedShortArray> usa = MakeArray<svtkUnsignedShortArray>("usa");
  svtkSmartPointer<svtkShortArray> sa = MakeArray<svtkShortArray>("sa");

  svtkSmartPointer<svtkUnsignedIntArray> uia = MakeArray<svtkUnsignedIntArray>("uia");
  svtkSmartPointer<svtkIntArray> ia = MakeArray<svtkIntArray>("ia");

  svtkSmartPointer<svtkUnsignedLongArray> ula = MakeArray<svtkUnsignedLongArray>("ula");
  svtkSmartPointer<svtkLongArray> la = MakeArray<svtkLongArray>("la");

  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->GetPointData()->AddArray(uca);
  polyData->GetPointData()->AddArray(ca);
  polyData->GetPointData()->AddArray(usa);
  polyData->GetPointData()->AddArray(sa);
  polyData->GetPointData()->AddArray(uia);
  polyData->GetPointData()->AddArray(ia);
  polyData->GetPointData()->AddArray(ula);
  polyData->GetPointData()->AddArray(la);

  svtkSmartPointer<svtkRadiusOutlierRemoval> outlierRemoval =
    svtkSmartPointer<svtkRadiusOutlierRemoval>::New();
  outlierRemoval->SetInputData(polyData);
  outlierRemoval->SetRadius(1.5);
  outlierRemoval->SetNumberOfNeighbors(2);
  outlierRemoval->Update();

  svtkPointData* inPD = polyData->GetPointData();
  svtkPointData* outPD = outlierRemoval->GetOutput()->GetPointData();
  // Number of arrays should match
  if (inPD->GetNumberOfArrays() != outPD->GetNumberOfArrays())
  {
    std::cout << "ERROR: Number of input arrays : " << inPD->GetNumberOfArrays()
              << " != " << outPD->GetNumberOfArrays() << std::endl;
    return EXIT_FAILURE;
  }
  // Types should not change
  int status = 0;
  for (int i = 0; i < outPD->GetNumberOfArrays(); ++i)
  {
    svtkDataArray* outArray = outPD->GetArray(i);
    svtkDataArray* inArray = inPD->GetArray(i);
    if (inArray->GetDataType() != outArray->GetDataType())
    {
      std::cout << "ERROR: Output array: " << outArray->GetName()
                << ", type: " << outArray->GetDataTypeAsString() << " does not match "
                << "Input array: " << inArray->GetName()
                << ", type: " << inArray->GetDataTypeAsString() << std::endl;
      ++status;
    }
  }
  return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

template <typename T>
svtkSmartPointer<T> MakeArray(const std::string& name)
{
  svtkSmartPointer<T> array = svtkSmartPointer<T>::New();
  array->SetName(name.c_str());
  array->SetNumberOfComponents(1);
  array->InsertNextValue(1);
  array->InsertNextValue(2);
  array->InsertNextValue(3);
  return array;
}
