/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParticleTracers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkImageGradient.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkSmartPointer.h"
#include "svtkStreamTracer.h"
#include <cassert>

int TestFieldNames(int, char*[])
{
  // create a multiblock data set of two images
  // with touching x extents so stream traces can
  // go from one to the other

  svtkNew<svtkRTAnalyticSource> source;
  source->SetWholeExtent(-10, 0, -10, 10, -10, 10);

  svtkNew<svtkImageGradient> gradient;
  gradient->SetDimensionality(3);
  gradient->SetInputConnection(source->GetOutputPort());
  gradient->Update();

  svtkSmartPointer<svtkImageData> image0 = svtkSmartPointer<svtkImageData>::New();
  image0->DeepCopy(svtkImageData::SafeDownCast(gradient->GetOutputDataObject(0)));
  image0->GetPointData()->SetActiveVectors("RTDataGradient");

  source->SetWholeExtent(0, 10, -10, 10, -10, 10);
  gradient->Update();

  svtkSmartPointer<svtkImageData> image1 = svtkSmartPointer<svtkImageData>::New();
  image1->DeepCopy(svtkImageData::SafeDownCast(gradient->GetOutputDataObject(0)));

  svtkIdType numPts = image0->GetNumberOfPoints();
  svtkSmartPointer<svtkDoubleArray> arr0 = svtkSmartPointer<svtkDoubleArray>::New();
  arr0->Allocate(numPts);
  arr0->SetNumberOfComponents(1);
  arr0->SetNumberOfTuples(image0->GetNumberOfPoints());
  for (svtkIdType idx = 0; idx < numPts; idx++)
  {
    arr0->SetTuple1(idx, 1.0);
  }
  arr0->SetName("array 0");
  image0->GetPointData()->AddArray(arr0);

  svtkSmartPointer<svtkDoubleArray> arr1 = svtkSmartPointer<svtkDoubleArray>::New();
  arr1->Allocate(numPts);
  arr1->SetName("array 1");
  for (svtkIdType idx = 0; idx < numPts; idx++)
  {
    arr1->SetTuple1(idx, 2.0);
  }
  image1->GetPointData()->AddArray(arr1);

  svtkNew<svtkMultiBlockDataSet> dataSets;
  dataSets->SetNumberOfBlocks(2);
  dataSets->SetBlock(0, image0);
  dataSets->SetBlock(1, image1);

  // create one seed
  svtkNew<svtkPolyData> seeds;
  svtkNew<svtkPoints> seedPoints;
  seedPoints->InsertNextPoint(-4.0, 0, 0);
  seeds->SetPoints(seedPoints);

  // perform the tracing and watch for warning
  svtkNew<svtkStreamTracer> tracer;
  tracer->SetSourceData(seeds);
  tracer->SetInputData(dataSets);
  tracer->SetMaximumPropagation(20.0);

  // run the tracing
  tracer->Update();

  // verify results
  svtkPolyData* trace = svtkPolyData::SafeDownCast(tracer->GetOutputDataObject(0));
  if (trace->GetPointData()->GetArray("array 0") != nullptr ||
    trace->GetPointData()->GetArray("array 1") != nullptr ||
    trace->GetPointData()->GetArray("RTData") == nullptr || trace->GetNumberOfPoints() == 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestStreamTracer(int n, char* a[])
{
  int numFailures(0);
  numFailures += TestFieldNames(n, a);
  return numFailures;
}
