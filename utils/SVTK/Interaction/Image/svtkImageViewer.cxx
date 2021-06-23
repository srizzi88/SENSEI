/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageViewer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageViewer.h"

#include "svtkActor2D.h"
#include "svtkCommand.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInteractorStyleImage.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageViewer);

//----------------------------------------------------------------------------
svtkImageViewer::svtkImageViewer()
{
  this->RenderWindow = svtkRenderWindow::New();
  this->Renderer = svtkRenderer::New();
  this->ImageMapper = svtkImageMapper::New();
  this->Actor2D = svtkActor2D::New();

  // setup the pipeline
  this->Actor2D->SetMapper(this->ImageMapper);
  this->Renderer->AddActor2D(this->Actor2D);
  this->RenderWindow->AddRenderer(this->Renderer);

  this->FirstRender = 1;

  this->Interactor = nullptr;
  this->InteractorStyle = nullptr;
}

//----------------------------------------------------------------------------
svtkImageViewer::~svtkImageViewer()
{
  this->ImageMapper->Delete();
  this->Actor2D->Delete();
  this->RenderWindow->Delete();
  this->Renderer->Delete();

  if (this->Interactor)
  {
    this->Interactor->Delete();
  }
  if (this->InteractorStyle)
  {
    this->InteractorStyle->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageMapper:\n";
  this->ImageMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RenderWindow:\n";
  this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Renderer:\n";
  this->Renderer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Actor2D:\n";
  this->Actor2D->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void svtkImageViewer::SetSize(int a[2])
{
  this->SetSize(a[0], a[1]);
}
//----------------------------------------------------------------------------
void svtkImageViewer::SetPosition(int a[2])
{
  this->SetPosition(a[0], a[1]);
}

//----------------------------------------------------------------------------
class svtkImageViewerCallback : public svtkCommand
{
public:
  static svtkImageViewerCallback* New() { return new svtkImageViewerCallback; }

  void Execute(svtkObject* caller, unsigned long event, void* svtkNotUsed(callData)) override
  {
    if (this->IV->GetInput() == nullptr)
    {
      return;
    }

    // Reset

    if (event == svtkCommand::ResetWindowLevelEvent)
    {
      this->IV->GetInputAlgorithm()->UpdateWholeExtent();
      double* range = this->IV->GetInput()->GetScalarRange();
      this->IV->SetColorWindow(range[1] - range[0]);
      this->IV->SetColorLevel(0.5 * (range[1] + range[0]));
      this->IV->Render();
      return;
    }

    // Start

    if (event == svtkCommand::StartWindowLevelEvent)
    {
      this->InitialWindow = this->IV->GetColorWindow();
      this->InitialLevel = this->IV->GetColorLevel();
      return;
    }

    // Adjust the window level here

    svtkInteractorStyleImage* isi = static_cast<svtkInteractorStyleImage*>(caller);

    const int* size = this->IV->GetRenderWindow()->GetSize();
    double window = this->InitialWindow;
    double level = this->InitialLevel;

    // Compute normalized delta

    double dx = 4.0 *
      (isi->GetWindowLevelCurrentPosition()[0] - isi->GetWindowLevelStartPosition()[0]) / size[0];
    double dy = 4.0 *
      (isi->GetWindowLevelStartPosition()[1] - isi->GetWindowLevelCurrentPosition()[1]) / size[1];

    // Scale by current values

    if (fabs(window) > 0.01)
    {
      dx = dx * window;
    }
    else
    {
      dx = dx * (window < 0 ? -0.01 : 0.01);
    }
    if (fabs(level) > 0.01)
    {
      dy = dy * level;
    }
    else
    {
      dy = dy * (level < 0 ? -0.01 : 0.01);
    }

    // Abs so that direction does not flip

    if (window < 0.0)
    {
      dx = -1 * dx;
    }
    if (level < 0.0)
    {
      dy = -1 * dy;
    }

    // Compute new window level

    double newWindow = dx + window;
    double newLevel;
    newLevel = level - dy;

    // Stay away from zero and really

    if (fabs(newWindow) < 0.01)
    {
      newWindow = 0.01 * (newWindow < 0 ? -1 : 1);
    }
    if (fabs(newLevel) < 0.01)
    {
      newLevel = 0.01 * (newLevel < 0 ? -1 : 1);
    }

    this->IV->SetColorWindow(newWindow);
    this->IV->SetColorLevel(newLevel);
    this->IV->Render();
  }

  svtkImageViewer* IV;
  double InitialWindow;
  double InitialLevel;
};

//----------------------------------------------------------------------------
void svtkImageViewer::SetupInteractor(svtkRenderWindowInteractor* rwi)
{
  if (this->Interactor && rwi != this->Interactor)
  {
    this->Interactor->Delete();
  }
  if (!this->InteractorStyle)
  {
    this->InteractorStyle = svtkInteractorStyleImage::New();
    svtkImageViewerCallback* cbk = svtkImageViewerCallback::New();
    cbk->IV = this;
    this->InteractorStyle->AddObserver(svtkCommand::WindowLevelEvent, cbk);
    this->InteractorStyle->AddObserver(svtkCommand::StartWindowLevelEvent, cbk);
    this->InteractorStyle->AddObserver(svtkCommand::ResetWindowLevelEvent, cbk);
    cbk->Delete();
  }

  if (!this->Interactor)
  {
    this->Interactor = rwi;
    rwi->Register(this);
  }
  this->Interactor->SetInteractorStyle(this->InteractorStyle);
  this->Interactor->SetRenderWindow(this->RenderWindow);
}

//----------------------------------------------------------------------------
void svtkImageViewer::Render()
{
  if (this->FirstRender)
  {
    // initialize the size if not set yet
    if (this->RenderWindow->GetSize()[0] == 0 && this->ImageMapper->GetInput())
    {
      // get the size from the mappers input
      this->ImageMapper->GetInputAlgorithm()->UpdateInformation();
      int* ext = this->ImageMapper->GetInputInformation()->Get(
        svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      // if it would be smaller than 150 by 100 then limit to 150 by 100
      int xs = ext[1] - ext[0] + 1;
      int ys = ext[3] - ext[2] + 1;
      this->RenderWindow->SetSize(xs < 150 ? 150 : xs, ys < 100 ? 100 : ys);
    }
    this->FirstRender = 0;
  }
  this->RenderWindow->Render();
}

//----------------------------------------------------------------------------
void svtkImageViewer::SetOffScreenRendering(svtkTypeBool i)
{
  this->RenderWindow->SetOffScreenRendering(i);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageViewer::GetOffScreenRendering()
{
  return this->RenderWindow->GetOffScreenRendering();
}

//----------------------------------------------------------------------------
void svtkImageViewer::OffScreenRenderingOn()
{
  this->SetOffScreenRendering(1);
}

//----------------------------------------------------------------------------
void svtkImageViewer::OffScreenRenderingOff()
{
  this->SetOffScreenRendering(0);
}

//----------------------------------------------------------------------------
svtkAlgorithm* svtkImageViewer::GetInputAlgorithm()
{
  return this->ImageMapper->GetInputAlgorithm();
}

//----------------------------------------------------------------------------
void svtkImageViewer::SetRenderWindow(svtkRenderWindow* renWin)
{
  svtkSetObjectBodyMacro(RenderWindow, svtkRenderWindow, renWin);
  renWin->AddRenderer(this->GetRenderer());
}
