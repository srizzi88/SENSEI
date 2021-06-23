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
#include "svtkMultiBlockDataSet.h"
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

int TestGlyph3DMapperTreeIndexing(int argc, char* argv[])
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
  s0->Update();
  s1->Update();
  s2->Update();

  // Combine the glyph sources into a single dataset:
  svtkNew<svtkMultiBlockDataSet> glyphTree;
  glyphTree->SetNumberOfBlocks(3);
  glyphTree->SetBlock(0, s0->GetOutputDataObject(0));
  glyphTree->SetBlock(1, s1->GetOutputDataObject(0));
  glyphTree->SetBlock(2, s2->GetOutputDataObject(0));

  svtkNew<svtkGlyph3DMapper> mapper;
  mapper->SetInputData(input);
  mapper->SetSourceTableTree(glyphTree);
  mapper->SetRange(0, 2);
  mapper->SetUseSourceTableTree(true);
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
  renderer->AutomaticLightCreationOff();
  renderer->RemoveAllLights();

  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  // Ensure the mapper works when no lights are available
  renderer->AutomaticLightCreationOff();
  renderer->RemoveAllLights();

  renWin->Render();

  renderer->AutomaticLightCreationOn();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
