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
#include "svtkTransform.h"
#include "svtkTransformFilter.h"
#include "svtkUnsignedCharArray.h"

int TestGlyph3DMapperTreeIndexingCompositeGlyphs(int argc, char* argv[])
{
  // The points to glyph:
  svtkNew<svtkPolyData> input;
  svtkNew<svtkPoints> points;
  svtkNew<svtkIntArray> indexArray;
  svtkNew<svtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(3);

  unsigned char color[3];
  for (int row = 0; row < 2; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      points->InsertNextPoint((row ? col : (2 - col)) * 5, row * 5, 0.);
      indexArray->InsertNextValue(col);

      color[0] = static_cast<unsigned char>(((row + 1) / 2.) * 255 + 0.5);
      color[1] = static_cast<unsigned char>(((col + 1) / 3.) * 255 + 0.5);
      color[2] = static_cast<unsigned char>(((row + col + 1) / 4.) * 255 + 0.5);
      colors->InsertNextTypedTuple(color);
    }
  }

  input->SetPoints(points);
  input->GetPointData()->AddArray(indexArray);
  indexArray->SetName("GlyphIndex");
  input->GetPointData()->AddArray(colors);
  colors->SetName("Colors");

  // The glyph sources:
  svtkNew<svtkArrowSource> s0a;
  svtkNew<svtkTransformFilter> s0b;
  svtkNew<svtkCubeSource> s1a;
  svtkNew<svtkTransformFilter> s1b;
  svtkNew<svtkSphereSource> s2a;
  svtkNew<svtkTransformFilter> s2b;

  svtkNew<svtkTransform> transform;
  transform->Identity();
  transform->RotateZ(45.);
  transform->Scale(0.5, 2, 1.0);
  transform->Translate(.5, .5, .5);

  s0b->SetInputConnection(s0a->GetOutputPort());
  s0b->SetTransform(transform);
  s1b->SetInputConnection(s1a->GetOutputPort());
  s1b->SetTransform(transform);
  s2b->SetInputConnection(s2a->GetOutputPort());
  s2b->SetTransform(transform);

  s0a->Update();
  s0b->Update();
  s1a->Update();
  s1b->Update();
  s2a->Update();
  s2b->Update();

  svtkNew<svtkMultiBlockDataSet> s0;
  s0->SetNumberOfBlocks(2);
  s0->SetBlock(0, s0a->GetOutputDataObject(0));
  s0->SetBlock(1, s0b->GetOutputDataObject(0));

  svtkNew<svtkMultiBlockDataSet> s1;
  s1->SetNumberOfBlocks(2);
  s1->SetBlock(0, s1a->GetOutputDataObject(0));
  s1->SetBlock(1, s1b->GetOutputDataObject(0));

  svtkNew<svtkMultiBlockDataSet> s2;
  s2->SetNumberOfBlocks(2);
  s2->SetBlock(0, s2a->GetOutputDataObject(0));
  s2->SetBlock(1, s2b->GetOutputDataObject(0));

  // Combine the glyph sources into a single dataset:
  svtkNew<svtkMultiBlockDataSet> glyphTree;
  glyphTree->SetNumberOfBlocks(3);
  glyphTree->SetBlock(0, s0);
  glyphTree->SetBlock(1, s1);
  glyphTree->SetBlock(2, s2);

  svtkNew<svtkGlyph3DMapper> mapper;
  mapper->SetInputData(input);
  mapper->SetSourceTableTree(glyphTree);
  mapper->SetRange(0, 2);
  mapper->SetUseSourceTableTree(true);
  mapper->SetSourceIndexing(true);
  mapper->SetSourceIndexArray("GlyphIndex");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Colors");

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
