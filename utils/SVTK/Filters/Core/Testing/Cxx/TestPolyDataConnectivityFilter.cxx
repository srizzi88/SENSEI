/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkAppendPolyData.h>
#include <svtkCellArray.h>
#include <svtkFloatArray.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkPointData.h>
#include <svtkPolyDataConnectivityFilter.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>

namespace
{
void InitializePolyData(svtkPolyData* polyData, int dataType)
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
  verts->InsertNextCell(4);
  svtkSmartPointer<svtkFloatArray> scalars = svtkSmartPointer<svtkFloatArray>::New();

  if (dataType == SVTK_DOUBLE)
  {
    points->SetDataType(SVTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }
  else
  {
    points->SetDataType(SVTK_FLOAT);
    for (unsigned int i = 0; i < 4; ++i)
    {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  scalars->Squeeze();
  polyData->GetPointData()->SetScalars(scalars);
  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
}

int FilterPolyDataConnectivity(int dataType, int outputPointsPrecision)
{
  svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  svtkSmartPointer<svtkPolyDataConnectivityFilter> polyDataConnectivityFilter =
    svtkSmartPointer<svtkPolyDataConnectivityFilter>::New();
  polyDataConnectivityFilter->SetOutputPointsPrecision(outputPointsPrecision);
  polyDataConnectivityFilter->ScalarConnectivityOn();
  polyDataConnectivityFilter->SetScalarRange(0.25, 0.75);
  polyDataConnectivityFilter->SetInputData(inputPolyData);

  polyDataConnectivityFilter->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = polyDataConnectivityFilter->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

bool MarkVisitedPoints()
{
  // Set up two disconnected spheres.
  svtkNew<svtkSphereSource> sphere1;
  sphere1->SetCenter(-1, 0, 0);
  sphere1->Update();
  svtkIdType numPtsSphere1 = sphere1->GetOutput()->GetNumberOfPoints();

  svtkNew<svtkSphereSource> sphere2;
  sphere2->SetCenter(1, 0, 0);
  sphere2->SetPhiResolution(32);

  svtkNew<svtkAppendPolyData> spheres;
  spheres->SetInputConnection(sphere1->GetOutputPort());
  spheres->AddInputConnection(sphere2->GetOutputPort());
  spheres->Update();

  // Test SVTK_EXTRACT_CLOSEST_POINT_REGION mode.
  // Select the one with the highest points IDS so we can ensure marked visited points
  // use the original indices.
  svtkPolyDataConnectivityFilter* connectivity = svtkPolyDataConnectivityFilter::New();
  connectivity->SetInputConnection(spheres->GetOutputPort());
  connectivity->SetExtractionModeToClosestPointRegion();
  connectivity->SetClosestPoint(1, 0, 0);
  connectivity->MarkVisitedPointIdsOn();
  connectivity->Update();

  // Check that the marked point IDs fall in the range of the second sphere, which is closest.
  svtkIdList* visitedPts = connectivity->GetVisitedPointIds();
  for (svtkIdType id = 0; id < visitedPts->GetNumberOfIds(); ++id)
  {
    svtkIdType visitedPt = visitedPts->GetId(id);
    if (visitedPts->GetId(id) < numPtsSphere1)
    {
      std::cerr << "Visited point id " << visitedPt << " is from sphere1 and not sphere2 "
                << "in SVTK_EXTRACT_CLOSEST_POINT_REGION mode." << std::endl;
      return false;
    }
  }

  connectivity->Delete();

  // Test SVTK_EXTRACT_SPECIFIED_REGIONS mode.
  connectivity = svtkPolyDataConnectivityFilter::New();
  connectivity->SetInputConnection(spheres->GetOutputPort());
  connectivity->SetExtractionModeToSpecifiedRegions();
  connectivity->InitializeSpecifiedRegionList();
  connectivity->AddSpecifiedRegion(1);
  connectivity->Update();

  // Check that the marked point IDs fall in the range of the second sphere, which is region 1.
  bool succeeded = true;
  visitedPts = connectivity->GetVisitedPointIds();
  for (svtkIdType id = 0; id < visitedPts->GetNumberOfIds(); ++id)
  {
    svtkIdType visitedPt = visitedPts->GetId(id);
    if (visitedPt < numPtsSphere1)
    {
      std::cerr << "Visited point id " << visitedPt << " is from sphere1 and not sphere2 "
                << "in SVTK_EXTRACT_SPECIFIED_REGIONS mode." << std::endl;
      succeeded = false;
    }
  }

  connectivity->Delete();

  // Test SVTK_EXTRACT_LARGEST_REGION mode.
  connectivity = svtkPolyDataConnectivityFilter::New();
  connectivity->SetInputConnection(spheres->GetOutputPort());
  connectivity->SetExtractionModeToLargestRegion();
  connectivity->Update();

  // Check that the marked point IDs fall in the range of the second sphere, which is biggest.
  visitedPts = connectivity->GetVisitedPointIds();
  for (svtkIdType id = 0; id < visitedPts->GetNumberOfIds(); ++id)
  {
    svtkIdType visitedPt = visitedPts->GetId(id);
    if (visitedPt < numPtsSphere1)
    {
      std::cerr << "Visited point id " << visitedPt << " is from sphere1 and not sphere2 "
                << "in SVTK_EXTRACT_SPECIFIED_REGIONS mode." << std::endl;
      succeeded = false;
    }
  }

  connectivity->Delete();

  return succeeded;
}
}

int TestPolyDataConnectivityFilter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int dataType = FilterPolyDataConnectivity(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  if (!MarkVisitedPoints())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
