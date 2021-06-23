/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipTriangulateAndInterpolate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// This code tests for the case when svtkBoxClipDataSet is given a collection of
// cells that it must triangulate and interpolate.  At one time there was a bug
// that sent the wrong indices for interpolating in this case.

#include "svtkActor.h"
#include "svtkBoxClipDataSet.h"
#include "svtkCellArray.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnstructuredGrid.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

const int NumImagesX = 6;
const int NumImagesY = 2;

static void CreateHex(svtkUnstructuredGrid* hex)
{
  SVTK_CREATE(svtkPoints, points);
  points->Allocate(24);
  points->InsertNextPoint(-0.5, -0.5, -0.5);
  points->InsertNextPoint(0.5, -0.5, -0.5);
  points->InsertNextPoint(0.5, 0.5, -0.5);
  points->InsertNextPoint(-0.5, 0.5, -0.5);
  points->InsertNextPoint(-0.5, -0.5, 0.5);
  points->InsertNextPoint(0.5, -0.5, 0.5);
  points->InsertNextPoint(0.5, 0.5, 0.5);
  points->InsertNextPoint(-0.5, 0.5, 0.5);
  hex->SetPoints(points);

  SVTK_CREATE(svtkCellArray, cells);
  cells->AllocateExact(1, 8);
  cells->InsertNextCell(8);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  cells->InsertCellPoint(4);
  cells->InsertCellPoint(5);
  cells->InsertCellPoint(6);
  cells->InsertCellPoint(7);
  hex->SetCells(SVTK_HEXAHEDRON, cells);

  SVTK_CREATE(svtkDoubleArray, data);
  data->SetName("data");
  data->SetNumberOfComponents(1);
  data->SetNumberOfTuples(8);
  data->SetValue(0, 0.0);
  data->SetValue(1, 0.0);
  data->SetValue(2, 1.0);
  data->SetValue(3, 1.0);
  data->SetValue(4, 0.0);
  data->SetValue(5, 0.0);
  data->SetValue(6, 1.0);
  data->SetValue(7, 1.0);
  hex->GetPointData()->SetScalars(data);
}

static void CreateQuad(svtkPolyData* quad)
{
  SVTK_CREATE(svtkPoints, points);
  points->Allocate(12);
  points->InsertNextPoint(-0.5, -0.5, 0.0);
  points->InsertNextPoint(0.5, -0.5, 0.0);
  points->InsertNextPoint(0.5, 0.5, 0.0);
  points->InsertNextPoint(-0.5, 0.5, 0.0);
  quad->SetPoints(points);

  SVTK_CREATE(svtkCellArray, cells);
  cells->AllocateExact(1, 4);
  cells->InsertNextCell(4);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  quad->SetPolys(cells);

  SVTK_CREATE(svtkDoubleArray, data);
  data->SetName("data");
  data->SetNumberOfComponents(1);
  data->SetNumberOfTuples(4);
  data->SetValue(0, 0.0);
  data->SetValue(1, 0.0);
  data->SetValue(2, 1.0);
  data->SetValue(3, 1.0);
  quad->GetPointData()->SetScalars(data);
}

static void CreateLine(svtkPolyData* line)
{
  SVTK_CREATE(svtkPoints, points);
  points->Allocate(12);
  points->InsertNextPoint(0.0, -0.5, 0.0);
  points->InsertNextPoint(0.0, -0.25, 0.0);
  points->InsertNextPoint(0.0, 0.25, 0.0);
  points->InsertNextPoint(0.0, 0.25, 0.0);
  line->SetPoints(points);

  SVTK_CREATE(svtkCellArray, cells);
  cells->AllocateExact(1, 4);
  cells->InsertNextCell(4);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  line->SetLines(cells);

  SVTK_CREATE(svtkDoubleArray, data);
  data->SetName("data");
  data->SetNumberOfComponents(1);
  data->SetNumberOfTuples(4);
  data->SetValue(0, 0.0);
  data->SetValue(1, 1.0);
  data->SetValue(2, 1.0);
  data->SetValue(3, 1.0);
  line->GetPointData()->SetScalars(data);
}

static void SetClipAsHexahedron(svtkBoxClipDataSet* clip, double xmin, double xmax, double ymin,
  double ymax, double zmin, double zmax)
{
  double lowPoint[3] = { xmin, ymin, zmin };
  double highPoint[3] = { xmax, ymax, zmax };
  const double negXVec[3] = { -1.0, 0.0, 0.0 };
  const double negYVec[3] = { 0.0, -1.0, 0.0 };
  const double negZVec[3] = { 0.0, 0.0, -1.0 };
  const double posXVec[3] = { 1.0, 0.0, 0.0 };
  const double posYVec[3] = { 0.0, 1.0, 0.0 };
  const double posZVec[3] = { 0.0, 0.0, 1.0 };

  clip->SetBoxClip(negXVec, lowPoint, negYVec, lowPoint, negZVec, lowPoint, posXVec, highPoint,
    posYVec, highPoint, posZVec, highPoint);
}

static void AddToRenderWindow(
  svtkRenderWindow* renwin, svtkBoxClipDataSet* boxclip, int x = 0, int y = 0)
{
  SVTK_CREATE(svtkRenderer, renderer);
  renderer->SetViewport(static_cast<double>(x) / NumImagesX, static_cast<double>(y) / NumImagesY,
    static_cast<double>(x + 1) / NumImagesX, static_cast<double>(y + 1) / NumImagesY);

  SVTK_CREATE(svtkDataSetSurfaceFilter, surface1);
  surface1->SetInputConnection(boxclip->GetOutputPort(0));

  SVTK_CREATE(svtkPolyDataMapper, mapper1);
  mapper1->SetInputConnection(surface1->GetOutputPort());
  mapper1->InterpolateScalarsBeforeMappingOn();

  SVTK_CREATE(svtkActor, actor1);
  actor1->SetMapper(mapper1);
  renderer->AddActor(actor1);

  SVTK_CREATE(svtkDataSetSurfaceFilter, surface2);
  surface2->SetInputConnection(boxclip->GetOutputPort(1));

  SVTK_CREATE(svtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(surface2->GetOutputPort());

  SVTK_CREATE(svtkActor, actor2);
  actor2->SetMapper(mapper2);
  renderer->AddActor(actor2);

  renwin->AddRenderer(renderer);
}

int BoxClipTriangulateAndInterpolate(int, char*[])
{
  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetSize(600, 400);

  SVTK_CREATE(svtkUnstructuredGrid, hex);
  CreateHex(hex);

  SVTK_CREATE(svtkPolyData, quad);
  CreateQuad(quad);

  SVTK_CREATE(svtkPolyData, line);
  CreateLine(line);

  SVTK_CREATE(svtkBoxClipDataSet, clip00);
  clip00->SetInputData(hex);
  clip00->SetBoxClip(0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip00, 0, 0);

  SVTK_CREATE(svtkBoxClipDataSet, clip01);
  clip01->SetInputData(hex);
  clip01->GenerateClippedOutputOn();
  clip01->SetBoxClip(0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip01, 0, 1);

  SVTK_CREATE(svtkBoxClipDataSet, clip10);
  clip10->SetInputData(hex);
  SetClipAsHexahedron(clip10, 0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip10, 1, 0);

  SVTK_CREATE(svtkBoxClipDataSet, clip11);
  clip11->SetInputData(hex);
  clip11->GenerateClippedOutputOn();
  SetClipAsHexahedron(clip11, 0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip11, 1, 1);

  SVTK_CREATE(svtkBoxClipDataSet, clip20);
  clip20->SetInputData(quad);
  clip20->SetBoxClip(0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip20, 2, 0);

  SVTK_CREATE(svtkBoxClipDataSet, clip21);
  clip21->SetInputData(quad);
  clip21->GenerateClippedOutputOn();
  clip21->SetBoxClip(0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip21, 2, 1);

  SVTK_CREATE(svtkBoxClipDataSet, clip30);
  clip30->SetInputData(quad);
  SetClipAsHexahedron(clip30, 0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip30, 3, 0);

  SVTK_CREATE(svtkBoxClipDataSet, clip31);
  clip31->SetInputData(quad);
  clip31->GenerateClippedOutputOn();
  SetClipAsHexahedron(clip31, 0.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip31, 3, 1);

  SVTK_CREATE(svtkBoxClipDataSet, clip40);
  clip40->SetInputData(line);
  clip40->SetBoxClip(-1.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip40, 4, 0);

  SVTK_CREATE(svtkBoxClipDataSet, clip41);
  clip41->SetInputData(line);
  clip41->GenerateClippedOutputOn();
  clip41->SetBoxClip(-1.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip41, 4, 1);

  SVTK_CREATE(svtkBoxClipDataSet, clip50);
  clip50->SetInputData(line);
  SetClipAsHexahedron(clip50, -1.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip50, 5, 0);

  SVTK_CREATE(svtkBoxClipDataSet, clip51);
  clip51->SetInputData(line);
  clip51->GenerateClippedOutputOn();
  SetClipAsHexahedron(clip51, -1.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  AddToRenderWindow(renwin, clip51, 5, 1);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
