/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCutter.h"
#include "svtkDataSetTriangleFilter.h"
#include "svtkImageDataToPointSet.h"
#include "svtkPlane.h"
#include "svtkPointDataToCellData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygonBuilder.h"
#include "svtkRTAnalyticSource.h"
#include "svtkSmartPointer.h"
#include <cassert>

bool TestStructured(int type)
{
  svtkSmartPointer<svtkRTAnalyticSource> imageSource = svtkSmartPointer<svtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2, 2, -2, 2, -2, 2);

  svtkSmartPointer<svtkAlgorithm> filter;
  if (type == 0)
  {
    filter = imageSource;
  }
  else
  {
    filter = svtkSmartPointer<svtkImageDataToPointSet>::New();
    filter->SetInputConnection(imageSource->GetOutputPort());
  }

  svtkSmartPointer<svtkCutter> cutter = svtkSmartPointer<svtkCutter>::New();
  svtkSmartPointer<svtkPlane> p3d = svtkSmartPointer<svtkPlane>::New();
  p3d->SetOrigin(-1.5, -1.5, -1.5);
  p3d->SetNormal(1, 1, 1);

  cutter->SetCutFunction(p3d);
  cutter->SetInputConnection(0, filter->GetOutputPort());

  cutter->SetGenerateTriangles(0);
  cutter->Update();
  svtkPolyData* output = svtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 4 || output->CheckAttributes())
  {
    return false;
  }

  cutter->SetGenerateTriangles(1);
  cutter->Update();
  output = svtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 7 || output->CheckAttributes())
  {
    return false;
  }
  return true;
}

bool TestUnstructured()
{
  svtkSmartPointer<svtkRTAnalyticSource> imageSource = svtkSmartPointer<svtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2, 2, -2, 2, -2, 2);

  svtkSmartPointer<svtkPointDataToCellData> dataFilter =
    svtkSmartPointer<svtkPointDataToCellData>::New();
  dataFilter->SetInputConnection(imageSource->GetOutputPort());

  svtkSmartPointer<svtkDataSetTriangleFilter> tetraFilter =
    svtkSmartPointer<svtkDataSetTriangleFilter>::New();
  tetraFilter->SetInputConnection(dataFilter->GetOutputPort());

  svtkSmartPointer<svtkCutter> cutter = svtkSmartPointer<svtkCutter>::New();
  svtkSmartPointer<svtkPlane> p3d = svtkSmartPointer<svtkPlane>::New();
  p3d->SetOrigin(-1.5, -1.5, -1.5);
  p3d->SetNormal(1, 1, 1);

  cutter->SetCutFunction(p3d);
  cutter->SetInputConnection(0, tetraFilter->GetOutputPort());

  cutter->SetGenerateTriangles(0);
  cutter->Update();
  svtkPolyData* output = svtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 7)
  {
    return false;
  }

  cutter->SetGenerateTriangles(1);
  cutter->Update();
  output = svtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 10)
  {
    return false;
  }
  return true;
}

int TestCutter(int, char*[])
{
  for (int type = 0; type < 2; type++)
  {
    if (!TestStructured(type))
    {
      cerr << "Cutting Structured failed" << endl;
      return EXIT_FAILURE;
    }
  }

  if (!TestUnstructured())
  {
    cerr << "Cutting Unstructured failed" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
