/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestProjectSphereFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkProjectSphereFilter.h"

#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkFloatArray.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkPoints.h"
#include "svtkSphereSource.h"

#include "svtkCellArray.h"
#include "svtkPolyData.h"
#include "svtkVertex.h"

#include "svtkCommand.h"
#include "svtkExecutive.h"
#include "svtkTestErrorObserver.h"

#include <sstream>

static int ComparePolyData(svtkPolyData* p1, svtkPolyData* p2);
int UnitTestProjectSphereFilter(int, char*[])
{
  int status = EXIT_SUCCESS;

  {
    // Print
    std::cout << "  Testing print...";
    std::ostringstream emptyPrint;
    svtkSmartPointer<svtkProjectSphereFilter> filter = svtkSmartPointer<svtkProjectSphereFilter>::New();
    double center[3];
    center[0] = 1.0;
    center[1] = 2.0;
    center[2] = 3.0;
    filter->SetCenter(center);
    filter->KeepPolePointsOff();
    filter->TranslateZOn();
    filter->Print(emptyPrint);
    std::cout << "PASSED" << std::endl;
  }
  {
    std::cout << "  Testing errors...";
    svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();
    svtkSmartPointer<svtkTest::ErrorObserver> executiveObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();

    svtkSmartPointer<svtkProjectSphereFilter> filter = svtkSmartPointer<svtkProjectSphereFilter>::New();
    filter->AddObserver(svtkCommand::ErrorEvent, errorObserver);
    filter->GetExecutive()->AddObserver(svtkCommand::ErrorEvent, executiveObserver);
    svtkSmartPointer<svtkPolyData> badPoly = svtkSmartPointer<svtkPolyData>::New();
    svtkSmartPointer<svtkVertex> aVertex = svtkSmartPointer<svtkVertex>::New();
    aVertex->GetPointIds()->SetId(0, 0);
    aVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
    svtkSmartPointer<svtkCellArray> vertices = svtkSmartPointer<svtkCellArray>::New();
    vertices->InsertNextCell(aVertex);
    badPoly->SetVerts(vertices);
    filter->SetInputData(badPoly);
    filter->Update();

    int status1 = errorObserver->CheckErrorMessage("Can only deal with svtkPolyData polys");
    if (status1 == 0)
    {
      std::cout << "PASSED" << std::endl;
    }
    else
    {
      status++;
      std::cout << "FAILED" << std::endl;
    }
  }
  {
    int status1;
    std::cout << "Testing compare polydata...";
    svtkSmartPointer<svtkSphereSource> source = svtkSmartPointer<svtkSphereSource>::New();
    source->Update();

    svtkSmartPointer<svtkPolyData> polyData1 = svtkSmartPointer<svtkPolyData>::New();
    polyData1->DeepCopy(source->GetOutput());

    svtkSmartPointer<svtkProjectSphereFilter> filter = svtkSmartPointer<svtkProjectSphereFilter>::New();
    filter->SetInputConnection(source->GetOutputPort());
    filter->Update();

    svtkSmartPointer<svtkPolyData> polyData2 = svtkSmartPointer<svtkPolyData>::New();
    polyData2->DeepCopy(source->GetOutput());

    status1 = ComparePolyData(polyData1, polyData2);
    if (status1 != 0)
    {
      std::cout << "Failed" << std::endl;
    }
    else
    {
      std::cout << "Passed" << std::endl;
    }
    status += status1;
  }
  return status;
}

int ComparePolyData(svtkPolyData* p1, svtkPolyData* p2)
{
  int status = 0;
  if (p1->GetNumberOfCells() != p2->GetNumberOfCells())
  {
    std::cout << "ERROR: ComparePolyData - p1->GetNumberOfCells() " << p1->GetNumberOfCells()
              << " != "
              << "p2->GetNumberOfCells() " << p2->GetNumberOfCells() << std::endl;
    status++;
  }
  svtkIdList* pointIdList1;
  svtkIdType* ptIds1;
  int numCellPts1;
  svtkIdList* pointIdList2;
  svtkIdType* ptIds2;
  int numCellPts2;

  svtkSmartPointer<svtkCellIterator> cellIter1 =
    svtkSmartPointer<svtkCellIterator>::Take(p1->NewCellIterator());
  svtkSmartPointer<svtkCellIterator> cellIter2 =
    svtkSmartPointer<svtkCellIterator>::Take(p2->NewCellIterator());
  for (cellIter1->InitTraversal(), cellIter2->InitTraversal(); cellIter1->IsDoneWithTraversal();
       cellIter1->GoToNextCell(), cellIter2->GoToNextCell())
  {
    pointIdList1 = cellIter1->GetPointIds();
    numCellPts1 = pointIdList1->GetNumberOfIds();
    ptIds1 = pointIdList1->GetPointer(0);
    pointIdList2 = cellIter2->GetPointIds();
    numCellPts2 = pointIdList2->GetNumberOfIds();
    ptIds2 = pointIdList2->GetPointer(0);

    if (numCellPts1 != numCellPts2)
    {
      std::cout << "numCellPts1 != numCellPts2 " << numCellPts1 << " != " << numCellPts2
                << std::endl;
      return 1;
    }
    for (svtkIdType i = 0; i < numCellPts1; ++i)
    {
      if (ptIds1[i] != ptIds2[i])
      {
        std::cout << ptIds1[i] << " != " << ptIds2[i] << std::endl;
      }
    }
  }
  svtkPointData* pointData1;
  pointData1 = p1->GetPointData();
  svtkPointData* pointData2;
  pointData2 = p2->GetPointData();

  svtkSmartPointer<svtkFloatArray> normals1 = svtkFloatArray::SafeDownCast(pointData1->GetNormals());
  svtkSmartPointer<svtkFloatArray> normals2 = svtkFloatArray::SafeDownCast(pointData2->GetNormals());

  for (svtkIdType i = 0; i < normals1->GetNumberOfTuples(); ++i)
  {
    double normal1[3], normal2[3];
    normals1->GetTuple(i, normal1);
    normals2->GetTuple(i, normal2);
    for (int j = 0; j < 3; ++j)
    {
      if (normal1[j] != normal2[j])
      {
        std::cout << "Cell: " << i << " normal1[" << j << "] != normal2 " << j << "] " << normal1[j]
                  << " != " << normal2[j] << std::endl;
        ++status;
      }
    }
  }
  return status;
}
