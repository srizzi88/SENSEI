/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDensifyPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDensifyPolyData.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkXMLPolyDataWriter.h"

#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

int TestDensifyPolyData(int, char*[])
{

  SVTK_CREATE(svtkPoints, boxPoints);
  boxPoints->InsertNextPoint(-0.5, -0.5, -0.5);
  boxPoints->InsertNextPoint(-0.5, -0.5, 0.5);
  boxPoints->InsertNextPoint(-0.5, 0.5, 0.5);
  boxPoints->InsertNextPoint(-0.5, 0.5, -0.5);
  boxPoints->InsertNextPoint(0.5, -0.5, -0.5);
  boxPoints->InsertNextPoint(0.5, 0.5, -0.5);
  boxPoints->InsertNextPoint(0.5, -0.5, 0.5);
  boxPoints->InsertNextPoint(0.5, 0.5, 0.023809850216);
  boxPoints->InsertNextPoint(0.5, 0.072707727551, 0.5);
  boxPoints->InsertNextPoint(-0.014212930575, 0.5, 0.5);

  SVTK_CREATE(svtkPolyData, boxPolydata);
  SVTK_CREATE(svtkCellArray, polys);
  boxPolydata->SetPolys(polys);
  boxPolydata->SetPoints(boxPoints);
  {
    svtkIdType ids[] = { 0, 1, 2, 3 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 4, ids);
  }
  {
    svtkIdType ids[] = { 4, 5, 7, 8, 6 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 5, ids);
  }
  {
    svtkIdType ids[] = { 0, 4, 6, 1 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 4, ids);
  }
  {
    svtkIdType ids[] = { 3, 2, 9, 7, 5 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 5, ids);
  }
  {
    svtkIdType ids[] = { 0, 3, 5, 4 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 4, ids);
  }
  {
    svtkIdType ids[] = { 1, 6, 8, 9, 2 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 5, ids);
  }
  {
    svtkIdType ids[] = { 7, 9, 8 };
    boxPolydata->InsertNextCell(SVTK_POLYGON, 3, ids);
  }

  SVTK_CREATE(svtkDensifyPolyData, densifyFilter);
  densifyFilter->SetInputData(boxPolydata);
  densifyFilter->SetNumberOfSubdivisions(2);

  SVTK_CREATE(svtkXMLPolyDataWriter, writer);
  writer->SetInputConnection(densifyFilter->GetOutputPort());
  writer->SetFileName("tessellatedBox.vtp");
  writer->SetDataModeToAscii();
  writer->Update();

  SVTK_CREATE(svtkSphereSource, sphere);
  SVTK_CREATE(svtkDensifyPolyData, densifyFilter2);
  densifyFilter2->SetInputConnection(sphere->GetOutputPort());
  densifyFilter2->SetNumberOfSubdivisions(1);

  // Throw the stuff on the screen.
  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->SetSize(800, 640);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  SVTK_CREATE(svtkPolyDataMapper, mapper1);
  mapper1->SetInputData(boxPolydata);

  SVTK_CREATE(svtkActor, actor1);
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetPointSize(3.0f);

  SVTK_CREATE(svtkRenderer, renderer1);
  renderer1->AddActor(actor1);
  renderer1->SetBackground(0.0, 0.5, 0.5);
  renderer1->SetViewport(0, 0, 0.5, 0.5);
  renwin->AddRenderer(renderer1);
  actor1->GetProperty()->SetRepresentationToWireframe();

  SVTK_CREATE(svtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(densifyFilter->GetOutputPort());

  SVTK_CREATE(svtkActor, actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetPointSize(3.0f);

  SVTK_CREATE(svtkRenderer, renderer2);
  renderer2->AddActor(actor2);
  renderer2->SetBackground(0.0, 0.5, 0.5);
  renderer2->SetViewport(0.5, 0.0, 1, 0.5);
  renwin->AddRenderer(renderer2);
  actor2->GetProperty()->SetRepresentationToWireframe();

  SVTK_CREATE(svtkPolyDataMapper, mapper3);
  mapper3->SetInputConnection(sphere->GetOutputPort());

  SVTK_CREATE(svtkActor, actor3);
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetPointSize(3.0f);

  SVTK_CREATE(svtkRenderer, renderer3);
  renderer3->AddActor(actor3);
  renderer3->SetBackground(0.0, 0.5, 0.5);
  renderer3->SetViewport(0, 0.5, 0.5, 1);
  renwin->AddRenderer(renderer3);
  actor3->GetProperty()->SetRepresentationToWireframe();

  SVTK_CREATE(svtkPolyDataMapper, mapper4);
  mapper4->SetInputConnection(densifyFilter2->GetOutputPort());

  SVTK_CREATE(svtkActor, actor4);
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetPointSize(3.0f);

  SVTK_CREATE(svtkRenderer, renderer4);
  renderer4->AddActor(actor4);
  renderer4->SetBackground(0.0, 0.5, 0.5);
  renderer4->SetViewport(0.5, 0.5, 1, 1);
  renwin->AddRenderer(renderer4);
  actor4->GetProperty()->SetRepresentationToWireframe();

  renwin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
