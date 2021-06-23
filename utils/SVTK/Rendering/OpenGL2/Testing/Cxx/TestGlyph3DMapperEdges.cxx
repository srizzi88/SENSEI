/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkElevationFilter.h"
#include "svtkGlyph3DMapper.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

int TestGlyph3DMapperEdges(int argc, char* argv[])
{
  int res = 1;
  svtkNew<svtkPlaneSource> plane;
  plane->SetResolution(res, res);

  svtkNew<svtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-1, -1, -1);
  colors->SetHighPoint(0.5, 0.5, 0.5);

  svtkNew<svtkSphereSource> squad;
  squad->SetPhiResolution(5);
  squad->SetThetaResolution(9);

  svtkNew<svtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetScaleFactor(1.2);
  glypher->SetSourceConnection(squad->GetOutputPort());

  svtkNew<svtkActor> glyphActor1;
  glyphActor1->SetMapper(glypher);
  glyphActor1->GetProperty()->SetEdgeVisibility(1);
  glyphActor1->GetProperty()->SetEdgeColor(1.0, 0.5, 0.5);
  // glyphActor1->GetProperty()->SetRenderLinesAsTubes(1);
  // glyphActor1->GetProperty()->SetLineWidth(5);

  // Standard rendering classes
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.2, 0.2, 0.2);
  renWin->SetSize(300, 300);

  renderer->AddActor(glyphActor1);

  ////////////////////////////////////////////////////////////

  // run the test

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
