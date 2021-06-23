/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReaderPointCloud.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkPLYReader
// .SECTION Description
//

#include "svtkPLYReader.h"
#include "svtkSmartPointer.h"

#include <svtkGlyph3D.h>
#include <svtkSphereSource.h>

#include "svtkActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

int TestPLYReaderPointCloud(int argc, char* argv[])
{
  // Read file name.
  const char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/PointCloud.ply");

  // Create the reader.
  svtkSmartPointer<svtkPLYReader> reader = svtkSmartPointer<svtkPLYReader>::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Create a mapper.
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Guess at a decent radius
  double bounds[6];
  reader->GetOutput()->GetBounds(bounds);

  double range[3];
  for (int i = 0; i < 3; ++i)
  {
    range[i] = bounds[2 * i + 1] - bounds[2 * i];
  }
  double radius = range[0] * .05;

  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource->SetRadius(radius);

  svtkSmartPointer<svtkGlyph3D> glyph3D = svtkSmartPointer<svtkGlyph3D>::New();
  glyph3D->SetInputConnection(reader->GetOutputPort());
  glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
  glyph3D->ScalingOff();
  glyph3D->SetColorModeToColorByScalar();
  glyph3D->Update();

  svtkSmartPointer<svtkPolyDataMapper> glyph3DMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  glyph3DMapper->SetInputConnection(glyph3D->GetOutputPort());

  svtkSmartPointer<svtkActor> glyph3DActor = svtkSmartPointer<svtkActor>::New();
  glyph3DActor->SetMapper(glyph3DMapper);

  // Basic visualization.
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(ren);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->AddActor(glyph3DActor);
  ren->SetBackground(.4, .5, .7);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
