/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderViewBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRenderViewBase.h"

#include "svtkCamera.h"
#include "svtkGenericRenderWindowInteractor.h"
#include "svtkInteractorObserver.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"

svtkStandardNewMacro(svtkRenderViewBase);

svtkRenderViewBase::svtkRenderViewBase()
{
  this->Renderer = svtkSmartPointer<svtkRenderer>::New();
  this->RenderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  this->RenderWindow->AddRenderer(this->Renderer);

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  this->SetInteractor(iren);
}

svtkRenderViewBase::~svtkRenderViewBase() = default;

svtkRenderer* svtkRenderViewBase::GetRenderer()
{
  return this->Renderer;
}

void svtkRenderViewBase::SetRenderer(svtkRenderer* newren)
{
  svtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  svtkCollectionSimpleIterator cookie;
  rens->InitTraversal(cookie);
  while (svtkRenderer* ren = rens->GetNextRenderer(cookie))
  {
    if (ren->GetLayer() < 2)
    {
      ren->SetRenderWindow(nullptr);
      this->RenderWindow->RemoveRenderer(ren);
    }
  }

  this->RenderWindow->AddRenderer(newren);
  this->Renderer = newren;
}

svtkRenderWindow* svtkRenderViewBase::GetRenderWindow()
{
  return this->RenderWindow;
}

void svtkRenderViewBase::SetRenderWindow(svtkRenderWindow* win)
{
  if (!win)
  {
    svtkErrorMacro(<< "SetRenderWindow called with a null window pointer."
                  << " That can't be right.");
    return;
  }

  // move renderers to new window
  svtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  while (rens->GetNumberOfItems())
  {
    svtkRenderer* ren = rens->GetFirstRenderer();
    ren->SetRenderWindow(nullptr);
    win->AddRenderer(ren);
    this->RenderWindow->RemoveRenderer(ren);
  }

  svtkSmartPointer<svtkInteractorObserver> style =
    this->GetInteractor() ? this->GetInteractor()->GetInteractorStyle() : nullptr;
  this->RenderWindow = win;
  if (this->GetInteractor())
  {
    this->GetInteractor()->SetInteractorStyle(style);
  }
  else if (style)
  {
    svtkGenericRenderWindowInteractor* iren = svtkGenericRenderWindowInteractor::New();
    win->SetInteractor(iren);
    iren->SetInteractorStyle(style);
    iren->Delete();
  }
}

svtkRenderWindowInteractor* svtkRenderViewBase::GetInteractor()
{
  return this->RenderWindow->GetInteractor();
}

void svtkRenderViewBase::SetInteractor(svtkRenderWindowInteractor* interactor)
{
  if (interactor == this->GetInteractor())
  {
    return;
  }

  svtkSmartPointer<svtkInteractorObserver> style =
    this->GetInteractor() ? this->GetInteractor()->GetInteractorStyle() : nullptr;
  this->RenderWindow->SetInteractor(interactor);

  if (this->GetInteractor())
  {
    this->GetInteractor()->SetInteractorStyle(style);
  }
  else if (style && this->RenderWindow)
  {
    svtkGenericRenderWindowInteractor* iren = svtkGenericRenderWindowInteractor::New();
    this->RenderWindow->SetInteractor(iren);
    iren->SetInteractorStyle(style);
    iren->Delete();
  }
}

void svtkRenderViewBase::Render()
{
  this->PrepareForRendering();
  this->RenderWindow->Render();
}

void svtkRenderViewBase::ResetCamera()
{
  this->PrepareForRendering();
  this->Renderer->ResetCamera();
}

void svtkRenderViewBase::ResetCameraClippingRange()
{
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
}

void svtkRenderViewBase::PrepareForRendering()
{
  this->Update();
}

void svtkRenderViewBase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderWindow: ";
  if (this->RenderWindow)
  {
    os << "\n";
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << "\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
