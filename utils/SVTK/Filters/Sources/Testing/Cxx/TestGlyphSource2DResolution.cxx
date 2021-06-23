/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGlyphSource2DResolution.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This tests the circle resolution parameter for svtkGlyphSource2D

#include "svtkActor2D.h"
#include "svtkFloatArray.h"
#include "svtkGlyph2D.h"
#include "svtkGlyphSource2D.h"
#include "svtkMinimalStandardRandomSequence.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

int TestGlyphSource2DResolution(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkPolyData> pd;
  svtkNew<svtkPoints> pts;

  svtkNew<svtkFloatArray> scalars;
  svtkNew<svtkFloatArray> vectors;
  vectors->SetNumberOfComponents(3);

  pd->SetPoints(pts);
  pd->GetPointData()->SetScalars(scalars);
  pd->GetPointData()->SetVectors(vectors);

  svtkNew<svtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(1);

  int size = 400;

  for (int i = 0; i < 100; ++i)
  {
    randomSequence->Next();
    double x = randomSequence->GetValue() * size;
    randomSequence->Next();
    double y = randomSequence->GetValue() * size;
    pts->InsertNextPoint(x, y, 0.0);
    randomSequence->Next();
    scalars->InsertNextValue(5.0 * randomSequence->GetValue());
    randomSequence->Next();
    double ihat = randomSequence->GetValue() * 2 - 1;
    randomSequence->Next();
    double jhat = randomSequence->GetValue() * 2 - 1;
    vectors->InsertNextTuple3(ihat, jhat, 0.0);
  }

  svtkNew<svtkGlyphSource2D> gs;
  gs->SetGlyphTypeToCircle();
  gs->SetScale(20);
  gs->FilledOff();
  gs->CrossOn();

  svtkNew<svtkGlyphSource2D> gs1;
  gs1->SetGlyphTypeToCircle();
  gs1->SetResolution(24);
  gs1->SetScale(30);
  gs1->FilledOn();
  gs1->CrossOff();

  svtkNew<svtkGlyphSource2D> gs2;
  gs2->SetGlyphTypeToCircle();
  gs2->SetResolution(6);
  gs2->SetScale(20);
  gs2->FilledOn();
  gs2->CrossOff();

  svtkNew<svtkGlyphSource2D> gs3;
  gs3->SetGlyphTypeToCircle();
  gs3->SetResolution(5);
  gs3->SetScale(30);
  gs3->FilledOff();
  gs3->CrossOn();

  svtkNew<svtkGlyphSource2D> gs4;
  gs4->SetGlyphTypeToCircle();
  gs4->SetResolution(100);
  gs4->SetScale(50);
  gs4->FilledOff();
  gs4->CrossOff();

  svtkNew<svtkGlyph2D> glypher;
  glypher->SetInputData(pd);
  glypher->SetSourceConnection(0, gs->GetOutputPort());
  glypher->SetSourceConnection(1, gs1->GetOutputPort());
  glypher->SetSourceConnection(2, gs2->GetOutputPort());
  glypher->SetSourceConnection(3, gs3->GetOutputPort());
  glypher->SetSourceConnection(4, gs4->GetOutputPort());
  glypher->SetIndexModeToScalar();
  glypher->SetRange(0, 5);
  glypher->SetScaleModeToScaleByVector();

  svtkNew<svtkPolyDataMapper2D> mapper;
  mapper->SetInputConnection(glypher->GetOutputPort());
  mapper->SetScalarRange(0, 5);

  svtkNew<svtkActor2D> glyphActor;
  glyphActor->SetMapper(mapper);

  // Create the RenderWindow, Renderer
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRenderer> ren;
  ren->AddActor2D(glyphActor);
  ren->SetBackground(0.3, 0.3, 0.3);
  ren->ResetCamera();

  renWin->SetSize(size + 1, size - 1); // NPOT size
  renWin->AddRenderer(ren);
  renWin->Render();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
