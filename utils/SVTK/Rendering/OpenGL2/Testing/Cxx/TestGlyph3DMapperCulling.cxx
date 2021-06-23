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

int TestGlyph3DMapperCulling(int argc, char* argv[])
{
  int res = 10;
  svtkNew<svtkPlaneSource> plane;
  plane->SetResolution(res, res);

  svtkNew<svtkSphereSource> squad;
  squad->SetPhiResolution(10);
  squad->SetThetaResolution(10);
  squad->SetRadius(0.05);

  svtkNew<svtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(plane->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());
  glypher->SetCullingAndLOD(true);
  glypher->SetNumberOfLOD(2);
  glypher->SetLODDistanceAndTargetReduction(0, 18.0, 0.2);
  glypher->SetLODDistanceAndTargetReduction(1, 20.0, 1.0);
  glypher->SetLODColoring(true);

  svtkNew<svtkActor> glyphActor;
  glyphActor->SetMapper(glypher);

  // Standard rendering classes
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  renderer->AddActor(glyphActor);

  renderer->GetActiveCamera()->Azimuth(45.0);
  renderer->GetActiveCamera()->Roll(20.0);
  renderer->ResetCamera();

  renWin->Render();

  svtkIdType maxLOD = glypher->GetMaxNumberOfLOD();
  if (maxLOD < 2)
  {
    cout << "This feature cannot be tested, this GPU only supports " << maxLOD << " LODs.\n";
    return EXIT_SUCCESS;
  }

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
