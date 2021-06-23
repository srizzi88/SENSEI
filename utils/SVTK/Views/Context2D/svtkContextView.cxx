/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContextView.h"

#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextScene.h"

#include "svtkContextActor.h"
#include "svtkContextInteractorStyle.h"
#include "svtkInteractorStyle.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkViewport.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkContextView);

svtkCxxSetObjectMacro(svtkContextView, Context, svtkContext2D);
svtkCxxSetObjectMacro(svtkContextView, Scene, svtkContextScene);

//----------------------------------------------------------------------------
svtkContextView::svtkContextView()
{
  this->Context = svtkSmartPointer<svtkContext2D>::New();
  svtkNew<svtkContextDevice2D> pd;
  this->Context->Begin(pd);

  svtkContextActor* actor = svtkContextActor::New();
  this->Renderer->AddActor(actor);
  actor->Delete();
  this->Scene = actor->GetScene(); // We keep a pointer to this for convenience
  // Should not need to do this...
  this->Scene->SetRenderer(this->Renderer);

  svtkContextInteractorStyle* style = svtkContextInteractorStyle::New();
  style->SetScene(this->Scene);
  this->GetInteractor()->SetInteractorStyle(style);
  style->Delete();

  // Single color background by default.
  this->Renderer->SetBackground(1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------
svtkContextView::~svtkContextView() = default;

//----------------------------------------------------------------------------
svtkContext2D* svtkContextView::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
svtkContextScene* svtkContextView::GetScene()
{
  return this->Scene;
}

//----------------------------------------------------------------------------
void svtkContextView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
  {
    this->Context->PrintSelf(os, indent.GetNextIndent());
  }
}
