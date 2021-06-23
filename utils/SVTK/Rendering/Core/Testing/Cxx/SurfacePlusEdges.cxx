/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SurfacePlusEdges.cxx

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

// This test draws a sphere with the edges shown.  It also turns on coincident
// topology resolution with a z-shift to both make sure the wireframe is
// visible and to exercise that type of coincident topology resolution.

#include "svtkActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

int SurfacePlusEdges(int argc, char* argv[])
{
  svtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
  svtkMapper::SetResolveCoincidentTopologyZShift(0.1);

  SVTK_CREATE(svtkSphereSource, sphere);

  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(sphere->GetOutputPort());

  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.0);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  renwin->SetSize(250, 250);
  renwin->SetMultiSamples(0);

  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    SVTK_CREATE(svtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (retVal == svtkRegressionTester::PASSED) ? 0 : 1;
}
