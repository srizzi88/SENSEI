/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGlyph3DMapper.h"

#include "svtkActor.h"
#include "svtkArrowSource.h"
#include "svtkCubeSource.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

int TestGlyph3DMapperIndexing(int argc, char* argv[])
{
  // The points to glyph:
  svtkNew<svtkPolyData> input;
  svtkNew<svtkPoints> points;
  svtkNew<svtkIntArray> indexArray;

  for (int row = 0; row < 2; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      points->InsertNextPoint((row ? col : (2 - col)) * 5, row * 5, 0.);
      indexArray->InsertNextValue(col);
    }
  }

  input->SetPoints(points);
  input->GetPointData()->AddArray(indexArray);
  indexArray->SetName("GlyphIndex");

  // The glyph sources:
  svtkNew<svtkArrowSource> s0;
  svtkNew<svtkCubeSource> s1;
  svtkNew<svtkSphereSource> s2;

  svtkNew<svtkGlyph3DMapper> mapper;
  mapper->SetInputData(input);
  mapper->SetSourceConnection(0, s0->GetOutputPort());
  mapper->SetSourceConnection(1, s1->GetOutputPort());
  mapper->SetSourceConnection(2, s2->GetOutputPort());
  mapper->SetRange(0, 2);
  mapper->SetSourceIndexing(true);
  mapper->SetSourceIndexArray("GlyphIndex");

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1., 0., 0.);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0., 0., 0.);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();

  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
