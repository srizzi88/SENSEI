/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetRegionSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCellArray.h>
#include <svtkCellData.h>
// #include <svtkCellIterator.h>
#include <svtkDataSetRegionSurfaceFilter.h>
// #include <svtkDoubleArray.h>
// #include <svtkGenericCell.h>
#include <svtkIntArray.h>
#include <svtkPointData.h>
#include <svtkPointLocator.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkTetra.h>
#include <svtkTriangle.h>
#include <svtkUnstructuredGrid.h>
#include <svtkVersion.h>

namespace
{
void AddTetra(const double* p0, const double* p1, const double* p2, const double* p3,
  svtkPointLocator* pointLocator, svtkCellArray* cells)
{
  svtkSmartPointer<svtkTetra> t = svtkSmartPointer<svtkTetra>::New();
  static svtkIdType bIndices[4][4] = { { 0, 0, 0, 1 }, { 1, 0, 0, 0 }, { 0, 1, 0, 0 },
    { 0, 0, 1, 0 } };
  svtkIdType order = 1;
  svtkIdType nPoints = 4;
  t->GetPointIds()->SetNumberOfIds(nPoints);
  t->GetPoints()->SetNumberOfPoints(nPoints);
  t->Initialize();
  double p[3];
  svtkIdType pId;
  svtkIdType* bIndex;
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    bIndex = bIndices[i];
    for (svtkIdType j = 0; j < 3; j++)
    {
      p[j] = (p0[j] * bIndex[3]) / order + (p1[j] * bIndex[0]) / order +
        (p2[j] * bIndex[1]) / order + (p3[j] * bIndex[2]) / order;
    }
    pointLocator->InsertUniquePoint(p, pId);
    t->GetPointIds()->SetId(i, pId);
  }
  cells->InsertNextCell(t);
}
}

int TestDataSetRegionSurfaceFilter(int argc, char* argv[])
{
  // This test constructs a meshed cube comprised of linear tetrahedra and
  // assigns a material ID to each cell according to the octant in which it
  // lies. It then applies the svtkDataSetRegionSurfaceFilter and visualizes the
  // results.

  svtkIdType nX = 2;
  svtkIdType nY = 2;
  svtkIdType nZ = 2;

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();

  svtkSmartPointer<svtkPoints> pointArray = svtkSmartPointer<svtkPoints>::New();

  svtkSmartPointer<svtkPointLocator> pointLocator = svtkSmartPointer<svtkPointLocator>::New();
  double bounds[6] = { -1., 1., -1., 1., -1., 1. };
  pointLocator->InitPointInsertion(pointArray, bounds);

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();

  double p[8][3];
  double dx = (bounds[1] - bounds[0]) / nX;
  double dy = (bounds[3] - bounds[2]) / nY;
  double dz = (bounds[5] - bounds[4]) / nZ;
  for (svtkIdType i = 0; i < 8; i++)
  {
    for (svtkIdType j = 0; j < 3; j++)
    {
      p[i][j] = bounds[2 * j];
    }
  }
  p[1][0] += dx;
  p[2][0] += dx;
  p[2][1] += dy;
  p[3][1] += dy;
  p[5][0] += dx;
  p[5][2] += dz;
  p[6][0] += dx;
  p[6][1] += dy;
  p[6][2] += dz;
  p[7][1] += dy;
  p[7][2] += dz;

  svtkSmartPointer<svtkIntArray> region = svtkSmartPointer<svtkIntArray>::New();
  region->SetName("Regions");
  region->SetNumberOfTuples(5 * nX * nY * nZ);
  int counter = 0;

  for (svtkIdType xInc = 0; xInc < nX; xInc++)
  {
    p[0][1] = p[1][1] = p[4][1] = p[5][1] = bounds[2];
    p[2][1] = p[3][1] = p[6][1] = p[7][1] = bounds[2] + dy;

    for (svtkIdType yInc = 0; yInc < nY; yInc++)
    {
      p[0][2] = p[1][2] = p[2][2] = p[3][2] = bounds[4];
      p[4][2] = p[5][2] = p[6][2] = p[7][2] = bounds[4] + dz;

      for (svtkIdType zInc = 0; zInc < nZ; zInc++)
      {
        AddTetra(p[0], p[1], p[2], p[5], pointLocator, cellArray);
        AddTetra(p[0], p[2], p[3], p[7], pointLocator, cellArray);
        AddTetra(p[0], p[5], p[7], p[4], pointLocator, cellArray);
        AddTetra(p[2], p[5], p[6], p[7], pointLocator, cellArray);
        AddTetra(p[0], p[2], p[5], p[7], pointLocator, cellArray);

        int r = 4 * (2 * xInc / nX) + 2 * (2 * yInc / nY) + (2 * zInc / nZ);
        for (svtkIdType i = 0; i < 5; i++)
        {
          region->SetTypedTuple(counter++, &r);
        }

        for (svtkIdType i = 0; i < 8; i++)
        {
          p[i][2] += dz;
        }
      }

      for (svtkIdType i = 0; i < 8; i++)
      {
        p[i][1] += dy;
      }
    }

    for (svtkIdType i = 0; i < 8; i++)
    {
      p[i][0] += dx;
    }
  }

  unstructuredGrid->SetPoints(pointArray);
  unstructuredGrid->SetCells(SVTK_TETRA, cellArray);

  svtkIdType nPoints = unstructuredGrid->GetPoints()->GetNumberOfPoints();

  double maxDist = 0;
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    double xyz[3];
    unstructuredGrid->GetPoints()->GetPoint(i, xyz);
    double dist = sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
    maxDist = (dist > maxDist ? dist : maxDist);
  }

  unstructuredGrid->GetCellData()->AddArray(region);
  unstructuredGrid->GetCellData()->SetScalars(region);

  // Visualize
  svtkSmartPointer<svtkDataSetRegionSurfaceFilter> surfaceFilter =
    svtkSmartPointer<svtkDataSetRegionSurfaceFilter>::New();
  surfaceFilter->SetRegionArrayName("Regions");
  surfaceFilter->SetInputData(unstructuredGrid);

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());
  mapper->SetScalarRange(0, 7);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkCamera> camera = svtkSmartPointer<svtkCamera>::New();
  camera->SetPosition(3. * maxDist, 3. * maxDist, -3. * maxDist);
  camera->SetFocalPoint(.0, .0, 0.);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->SetActiveCamera(camera);

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
