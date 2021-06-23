/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRandomHyperTreeGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRandomHyperTreeGridSource.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"

#include <sstream>

namespace
{

double colors[8][3] = { { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.7, 0.3, 0.3 } };

void ConstructScene(svtkRenderer* renderer, int numPieces)
{
  for (int i = 0; i < numPieces; ++i)
  {
    svtkNew<svtkRandomHyperTreeGridSource> source;
    source->SetDimensions(5, 5, 2); // GridCell 4, 4, 1
    source->SetSeed(3713971);
    source->SetSplitFraction(0.75);

    svtkNew<svtkHyperTreeGridGeometry> geom;
    geom->SetInputConnection(source->GetOutputPort());

    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geom->GetOutputPort());
    mapper->SetPiece(i);
    mapper->SetNumberOfPieces(numPieces);

    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetColor(colors[i]);

    renderer->AddActor(actor);
  }

  std::ostringstream labelStr;
  labelStr << "NumPieces: " << numPieces;

  svtkNew<svtkTextActor> label;
  label->SetInput(labelStr.str().c_str());
  label->GetTextProperty()->SetVerticalJustificationToBottom();
  label->GetTextProperty()->SetJustificationToCentered();
  label->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  label->GetPositionCoordinate()->SetValue(0.5, 0.);
  renderer->AddActor(label);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
}

} // end anon namespace

int TestRandomHyperTreeGridSource(int, char*[])
{
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);

  {
    svtkNew<svtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    ConstructScene(renderer, 1);
    renWin->AddRenderer(renderer.Get());
  }

  {
    svtkNew<svtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    ConstructScene(renderer, 2);
    renWin->AddRenderer(renderer.Get());
  }

  {
    svtkNew<svtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.0, 0.5, 0.5);
    ConstructScene(renderer, 4);
    renWin->AddRenderer(renderer.Get());
  }

  {
    svtkNew<svtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.0, 1.0, 0.5);
    ConstructScene(renderer, 8);
    renWin->AddRenderer(renderer.Get());
  }

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
