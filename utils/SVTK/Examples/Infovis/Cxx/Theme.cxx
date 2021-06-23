/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Theme.cxx

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
#include "svtkViewTheme.h"

int main(int, char*[])
{
  svtkRandomGraphSource* source = svtkRandomGraphSource::New();

  svtkGraphLayoutView* view = svtkGraphLayoutView::New();
  view->SetRepresentationFromInputConnection(source->GetOutputPort());

  svtkViewTheme* theme = svtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  theme->Delete();
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("VertexDegree");
  view->SetVertexLabelVisibility(true);

  view->ResetCamera();
  view->Render();
  view->GetInteractor()->Start();

  source->Delete();
  view->Delete();

  return 0;
}
