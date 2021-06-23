/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDynamic2DLabelMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkDynamic2DLabelMapper

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkDynamic2DLabelMapper.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"
#include "svtkVariant.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestDynamic2DLabelMapper(int argc, char* argv[])
{
  svtkIdType numPoints = 75;

  SVTK_CREATE(svtkPolyData, poly);
  SVTK_CREATE(svtkPoints, pts);
  SVTK_CREATE(svtkCellArray, cells);
  cells->AllocateEstimate(numPoints, 1);
  pts->SetNumberOfPoints(numPoints);
  double x[3];
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    double v = 20.0 * static_cast<double>(i) / numPoints;
    x[0] = v * cos(v);
    x[1] = v * sin(v);
    x[2] = 0;
    pts->SetPoint(i, x);

    cells->InsertNextCell(1, &i);
  }
  poly->SetPoints(pts);
  poly->SetVerts(cells);

  SVTK_CREATE(svtkStringArray, nameArray);
  nameArray->SetName("name");
  for (svtkIdType i = 0; i < numPoints; i++)
  {
    nameArray->InsertNextValue(svtkVariant(i).ToString());
  }
  poly->GetPointData()->AddArray(nameArray);

  SVTK_CREATE(svtkDynamic2DLabelMapper, mapper);
  mapper->SetInputData(poly);
  SVTK_CREATE(svtkActor2D, actor);
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkPolyDataMapper, polyMapper);
  polyMapper->SetInputData(poly);
  SVTK_CREATE(svtkActor, polyActor);
  polyActor->SetMapper(polyMapper);

  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(actor);
  ren->AddActor(polyActor);
  SVTK_CREATE(svtkRenderWindow, win);
  win->AddRenderer(ren);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  ren->ResetCamera();
  win->Render();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
