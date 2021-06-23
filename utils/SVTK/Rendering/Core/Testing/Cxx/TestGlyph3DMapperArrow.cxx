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
#include "svtkArrowSource.h"
#include "svtkCamera.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkExecutive.h"
#include "svtkExtractGrid.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiBlockPLOT3DReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
//#define USE_FILTER

#ifdef USE_FILTER
#include "svtkGlyph3D.h"
#else
#include "svtkGlyph3DMapper.h"
#endif

// from Graphics/Testing/Python/glyphComb.py

int TestGlyph3DMapperArrow(int argc, char* argv[])
{
  svtkMultiBlockPLOT3DReader* reader = svtkMultiBlockPLOT3DReader::New();
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  reader->SetXYZFileName(fname);
  delete[] fname;
  fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");
  reader->SetQFileName(fname);
  delete[] fname;
  reader->SetScalarFunctionNumber(100);
  reader->SetVectorFunctionNumber(202);
  reader->Update();

  svtkExtractGrid* eg = svtkExtractGrid::New();
  eg->SetInputData(reader->GetOutput()->GetBlock(0));
  reader->Delete();
  eg->SetSampleRate(4, 4, 4);
  eg->Update();

  cout << "eg pts=" << eg->GetOutput()->GetNumberOfPoints() << endl;
  cout << "eg cells=" << eg->GetOutput()->GetNumberOfCells() << endl;

  // create simple poly data so we can apply glyph
  svtkArrowSource* arrow = svtkArrowSource::New();
  arrow->Update();
  cout << "pts=" << arrow->GetOutput()->GetNumberOfPoints() << endl;
  cout << "cells=" << arrow->GetOutput()->GetNumberOfCells() << endl;

#ifdef USE_FILTER
  svtkGlyph3D* glypher = svtkGlyph3D::New();
#else
  svtkGlyph3DMapper* glypher = svtkGlyph3DMapper::New();
#endif
  glypher->SetInputConnection(eg->GetOutputPort());
  eg->Delete();
  glypher->SetSourceConnection(arrow->GetOutputPort());
  glypher->SetScaleFactor(2.0);
  arrow->Delete();

#ifdef USE_FILTER
  svtkPolyDataMapper* glyphMapper = svtkPolyDataMapper::New();
  glyphMapper->SetInputConnection(glypher->GetOutputPort());
#endif

  svtkActor* glyphActor = svtkActor::New();
#ifdef USE_FILTER
  glyphActor->SetMapper(glyphMapper);
  glyphMapper->Delete();
#else
  glyphActor->SetMapper(glypher);
#endif
  glypher->Delete();

  // Create the rendering stuff

  svtkRenderer* ren = svtkRenderer::New();
  svtkRenderWindow* win = svtkRenderWindow::New();
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  ren->Delete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(win);
  win->Delete();

  ren->AddActor(glyphActor);
  glyphActor->Delete();
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);

  svtkCamera* cam = ren->GetActiveCamera();
  cam->SetClippingRange(3.95297, 50);
  cam->SetFocalPoint(8.88908, 0.595038, 29.3342);
  cam->SetPosition(-12.3332, 31.7479, 41.2387);
  cam->SetViewUp(0.060772, -0.319905, 0.945498);

  svtkTimerLog* timer = svtkTimerLog::New();
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "first frame: " << timer->GetElapsedTime() << " seconds" << endl;

  //  ren->GetActiveCamera()->Zoom(1.5);
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "second frame: " << timer->GetElapsedTime() << " seconds" << endl;
  timer->Delete();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
