/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNrrdReader.cxx

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

#include "svtkNrrdReader.h"

#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkImageMapper.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkTestUtilities.h"

int TestNrrdReader(int argc, char* argv[])
{
  char* filename1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.nrrd");
  char* filename2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.ascii.nhdr");
  if ((filename1 == nullptr) || (filename2 == nullptr))
  {
    cerr << "Could not get file names.";
    return 1;
  }

  svtkNew<svtkNrrdReader> reader1;
  if (!reader1->CanReadFile(filename1))
  {
    cerr << "Reader reports " << filename1 << " cannot be read.";
    return 1;
  }
  reader1->SetFileName(filename1);
  reader1->Update();

  svtkNew<svtkImageMapper> mapper1;
  mapper1->SetInputConnection(reader1->GetOutputPort());
  mapper1->SetColorWindow(256);
  mapper1->SetColorLevel(127.5);

  svtkNew<svtkActor2D> actor1;
  actor1->SetMapper(mapper1);

  svtkNew<svtkRenderer> renderer1;
  renderer1->AddActor(actor1);

  svtkNew<svtkNrrdReader> reader2;
  if (!reader2->CanReadFile(filename2))
  {
    cerr << "Reader reports " << filename2 << " cannot be read.";
    return 1;
  }
  reader2->SetFileName(filename2);
  reader2->Update();

  svtkNew<svtkImageMapper> mapper2;
  mapper2->SetInputConnection(reader2->GetOutputPort());
  mapper2->SetColorWindow(1.0);
  mapper2->SetColorLevel(0.5);

  svtkNew<svtkActor2D> actor2;
  actor2->SetMapper(mapper2);

  svtkNew<svtkRenderer> renderer2;
  renderer2->AddActor(actor2);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(200, 100);

  renderer1->SetViewport(0.0, 0.0, 0.5, 1.0);
  renderWindow->AddRenderer(renderer1);

  renderer2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renderWindow->AddRenderer(renderer2);

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  delete[] filename1;
  delete[] filename2;

  return (retVal != svtkRegressionTester::PASSED);
}
