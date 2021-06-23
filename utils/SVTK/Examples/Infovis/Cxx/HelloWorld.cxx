/*=========================================================================

  Program:   Visualization Toolkit
  Module:    HelloWorld.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example...
//

#include "svtkGraphLayoutView.h"
#include "svtkRandomGraphSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"

int main(int, char*[])
{
  svtkRandomGraphSource* source = svtkRandomGraphSource::New();

  svtkGraphLayoutView* view = svtkGraphLayoutView::New();
  view->SetRepresentationFromInputConnection(source->GetOutputPort());

  view->ResetCamera();
  view->Render();
  view->GetInteractor()->Start();

  source->Delete();
  view->Delete();

  return 0;
}
