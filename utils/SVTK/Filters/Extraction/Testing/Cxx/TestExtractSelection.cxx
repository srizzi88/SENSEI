/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkExtractSelectedPolyDataIds.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSphereSource.h"

#include "svtkRegressionTestImage.h"

#include <cassert>

int TestExtractSelection(int argc, char* argv[])
{
  svtkSelection* sel = svtkSelection::New();
  svtkSelectionNode* node = svtkSelectionNode::New();
  sel->AddNode(node);
  node->GetProperties()->Set(svtkSelectionNode::CONTENT_TYPE(), svtkSelectionNode::INDICES);
  node->GetProperties()->Set(svtkSelectionNode::FIELD_TYPE(), svtkSelectionNode::CELL);

  // Get types as strings
  assert(strcmp(svtkSelectionNode::GetContentTypeAsString(node->GetContentType()), "INDICES") == 0);
  assert(strcmp(svtkSelectionNode::GetFieldTypeAsString(node->GetFieldType()), "CELL") == 0);

  std::cout << *node << std::endl;

  // list of cells to be selected
  svtkIdTypeArray* arr = svtkIdTypeArray::New();
  arr->SetNumberOfTuples(4);
  arr->SetTuple1(0, 2);
  arr->SetTuple1(1, 4);
  arr->SetTuple1(2, 5);
  arr->SetTuple1(3, 8);

  node->SetSelectionList(arr);
  arr->Delete();

  svtkSphereSource* sphere = svtkSphereSource::New();

  svtkExtractSelectedPolyDataIds* selFilter = svtkExtractSelectedPolyDataIds::New();
  selFilter->SetInputData(1, sel);
  selFilter->SetInputConnection(0, sphere->GetOutputPort());
  sel->Delete();
  node->Delete();

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(selFilter->GetOutputPort());

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  svtkRenderer* ren = svtkRenderer::New();
  ren->AddActor(actor);

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren);

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean up
  sphere->Delete();
  selFilter->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
