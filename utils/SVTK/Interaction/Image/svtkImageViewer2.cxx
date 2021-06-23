/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageViewer2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageViewer2.h"

#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapToWindowLevelColors.h"
#include "svtkImageMapper3D.h"
#include "svtkInformation.h"
#include "svtkInteractorStyleImage.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageViewer2);

//----------------------------------------------------------------------------
svtkImageViewer2::svtkImageViewer2()
{
  this->RenderWindow = nullptr;
  this->Renderer = nullptr;
  this->ImageActor = svtkImageActor::New();
  this->WindowLevel = svtkImageMapToWindowLevelColors::New();
  this->Interactor = nullptr;
  this->InteractorStyle = nullptr;

  this->Slice = 0;
  this->FirstRender = 1;
  this->SliceOrientation = svtkImageViewer2::SLICE_ORIENTATION_XY;

  // Setup the pipeline

  svtkRenderWindow* renwin = svtkRenderWindow::New();
  this->SetRenderWindow(renwin);
  renwin->Delete();

  svtkRenderer* ren = svtkRenderer::New();
  this->SetRenderer(ren);
  ren->Delete();

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
svtkImageViewer2::~svtkImageViewer2()
{
  if (this->WindowLevel)
  {
    this->WindowLevel->Delete();
    this->WindowLevel = nullptr;
  }

  if (this->ImageActor)
  {
    this->ImageActor->Delete();
    this->ImageActor = nullptr;
  }

  if (this->Renderer)
  {
    this->Renderer->Delete();
    this->Renderer = nullptr;
  }

  if (this->RenderWindow)
  {
    this->RenderWindow->Delete();
    this->RenderWindow = nullptr;
  }

  if (this->Interactor)
  {
    this->Interactor->Delete();
    this->Interactor = nullptr;
  }

  if (this->InteractorStyle)
  {
    this->InteractorStyle->Delete();
    this->InteractorStyle = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetupInteractor(svtkRenderWindowInteractor* arg)
{
  if (this->Interactor == arg)
  {
    return;
  }

  this->UnInstallPipeline();

  if (this->Interactor)
  {
    this->Interactor->UnRegister(this);
  }

  this->Interactor = arg;

  if (this->Interactor)
  {
    this->Interactor->Register(this);
  }

  this->InstallPipeline();

  if (this->Renderer)
  {
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetRenderWindow(svtkRenderWindow* arg)
{
  if (this->RenderWindow == arg)
  {
    return;
  }

  this->UnInstallPipeline();

  if (this->RenderWindow)
  {
    this->RenderWindow->UnRegister(this);
  }

  this->RenderWindow = arg;

  if (this->RenderWindow)
  {
    this->RenderWindow->Register(this);
  }

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetRenderer(svtkRenderer* arg)
{
  if (this->Renderer == arg)
  {
    return;
  }

  this->UnInstallPipeline();

  if (this->Renderer)
  {
    this->Renderer->UnRegister(this);
  }

  this->Renderer = arg;

  if (this->Renderer)
  {
    this->Renderer->Register(this);
  }

  this->InstallPipeline();
  this->UpdateOrientation();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetSize(int width, int height)
{
  this->RenderWindow->SetSize(width, height);
}

//----------------------------------------------------------------------------
int* svtkImageViewer2::GetSize()
{
  return this->RenderWindow->GetSize();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::GetSliceRange(int& min, int& max)
{
  svtkAlgorithm* input = this->GetInputAlgorithm();
  if (input)
  {
    input->UpdateInformation();
    int* w_ext =
      input->GetOutputInformation(0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    min = w_ext[this->SliceOrientation * 2];
    max = w_ext[this->SliceOrientation * 2 + 1];
  }
}

//----------------------------------------------------------------------------
int* svtkImageViewer2::GetSliceRange()
{
  svtkAlgorithm* input = this->GetInputAlgorithm();
  if (input)
  {
    input->UpdateInformation();
    return input->GetOutputInformation(0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) +
      this->SliceOrientation * 2;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkImageViewer2::GetSliceMin()
{
  int* range = this->GetSliceRange();
  if (range)
  {
    return range[0];
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkImageViewer2::GetSliceMax()
{
  int* range = this->GetSliceRange();
  if (range)
  {
    return range[1];
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetSlice(int slice)
{
  int* range = this->GetSliceRange();
  if (range)
  {
    if (slice < range[0])
    {
      slice = range[0];
    }
    else if (slice > range[1])
    {
      slice = range[1];
    }
  }

  if (this->Slice == slice)
  {
    return;
  }

  this->Slice = slice;
  this->Modified();

  this->UpdateDisplayExtent();
  this->Render();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetSliceOrientation(int orientation)
{
  if (orientation < svtkImageViewer2::SLICE_ORIENTATION_YZ ||
    orientation > svtkImageViewer2::SLICE_ORIENTATION_XY)
  {
    svtkErrorMacro("Error - invalid slice orientation " << orientation);
    return;
  }

  if (this->SliceOrientation == orientation)
  {
    return;
  }

  this->SliceOrientation = orientation;

  // Update the viewer

  int* range = this->GetSliceRange();
  if (range)
  {
    this->Slice = static_cast<int>((range[0] + range[1]) * 0.5);
  }

  this->UpdateOrientation();
  this->UpdateDisplayExtent();

  if (this->Renderer && this->GetInput())
  {
    double scale = this->Renderer->GetActiveCamera()->GetParallelScale();
    this->Renderer->ResetCamera();
    this->Renderer->GetActiveCamera()->SetParallelScale(scale);
  }

  this->Render();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::UpdateOrientation()
{
  // Set the camera position

  svtkCamera* cam = this->Renderer ? this->Renderer->GetActiveCamera() : nullptr;
  if (cam)
  {
    switch (this->SliceOrientation)
    {
      case svtkImageViewer2::SLICE_ORIENTATION_XY:
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, 0, 1); // -1 if medical ?
        cam->SetViewUp(0, 1, 0);
        break;

      case svtkImageViewer2::SLICE_ORIENTATION_XZ:
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, -1, 0); // 1 if medical ?
        cam->SetViewUp(0, 0, 1);
        break;

      case svtkImageViewer2::SLICE_ORIENTATION_YZ:
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(1, 0, 0); // -1 if medical ?
        cam->SetViewUp(0, 0, 1);
        break;
    }
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer2::UpdateDisplayExtent()
{
  svtkAlgorithm* input = this->GetInputAlgorithm();
  if (!input || !this->ImageActor)
  {
    return;
  }

  input->UpdateInformation();
  svtkInformation* outInfo = input->GetOutputInformation(0);
  int* w_ext = outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  // Is the slice in range ? If not, fix it

  int slice_min = w_ext[this->SliceOrientation * 2];
  int slice_max = w_ext[this->SliceOrientation * 2 + 1];
  if (this->Slice < slice_min || this->Slice > slice_max)
  {
    this->Slice = static_cast<int>((slice_min + slice_max) * 0.5);
  }

  // Set the image actor

  switch (this->SliceOrientation)
  {
    case svtkImageViewer2::SLICE_ORIENTATION_XY:
      this->ImageActor->SetDisplayExtent(
        w_ext[0], w_ext[1], w_ext[2], w_ext[3], this->Slice, this->Slice);
      break;

    case svtkImageViewer2::SLICE_ORIENTATION_XZ:
      this->ImageActor->SetDisplayExtent(
        w_ext[0], w_ext[1], this->Slice, this->Slice, w_ext[4], w_ext[5]);
      break;

    case svtkImageViewer2::SLICE_ORIENTATION_YZ:
      this->ImageActor->SetDisplayExtent(
        this->Slice, this->Slice, w_ext[2], w_ext[3], w_ext[4], w_ext[5]);
      break;
  }

  // Figure out the correct clipping range

  if (this->Renderer)
  {
    if (this->InteractorStyle && this->InteractorStyle->GetAutoAdjustCameraClippingRange())
    {
      this->Renderer->ResetCameraClippingRange();
    }
    else
    {
      svtkCamera* cam = this->Renderer->GetActiveCamera();
      if (cam)
      {
        double bounds[6];
        this->ImageActor->GetBounds(bounds);
        double spos = bounds[this->SliceOrientation * 2];
        double cpos = cam->GetPosition()[this->SliceOrientation];
        double range = fabs(spos - cpos);
        double* spacing = outInfo->Get(svtkDataObject::SPACING());
        double avg_spacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;
        cam->SetClippingRange(range - avg_spacing * 3.0, range + avg_spacing * 3.0);
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetPosition(int x, int y)
{
  this->RenderWindow->SetPosition(x, y);
}

//----------------------------------------------------------------------------
int* svtkImageViewer2::GetPosition()
{
  return this->RenderWindow->GetPosition();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetDisplayId(void* a)
{
  this->RenderWindow->SetDisplayId(a);
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetWindowId(void* a)
{
  this->RenderWindow->SetWindowId(a);
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetParentId(void* a)
{
  this->RenderWindow->SetParentId(a);
}

//----------------------------------------------------------------------------
double svtkImageViewer2::GetColorWindow()
{
  return this->WindowLevel->GetWindow();
}

//----------------------------------------------------------------------------
double svtkImageViewer2::GetColorLevel()
{
  return this->WindowLevel->GetLevel();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetColorWindow(double s)
{
  this->WindowLevel->SetWindow(s);
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetColorLevel(double s)
{
  this->WindowLevel->SetLevel(s);
}

//----------------------------------------------------------------------------
class svtkImageViewer2Callback : public svtkCommand
{
public:
  static svtkImageViewer2Callback* New() { return new svtkImageViewer2Callback; }

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

  svtkImageViewer2* IV;
  double InitialWindow;
  double InitialLevel;
};

//----------------------------------------------------------------------------
void svtkImageViewer2::InstallPipeline()
{
  if (this->RenderWindow && this->Renderer)
  {
    this->RenderWindow->AddRenderer(this->Renderer);
  }

  if (this->Interactor)
  {
    if (!this->InteractorStyle)
    {
      this->InteractorStyle = svtkInteractorStyleImage::New();
      svtkImageViewer2Callback* cbk = svtkImageViewer2Callback::New();
      cbk->IV = this;
      this->InteractorStyle->AddObserver(svtkCommand::WindowLevelEvent, cbk);
      this->InteractorStyle->AddObserver(svtkCommand::StartWindowLevelEvent, cbk);
      this->InteractorStyle->AddObserver(svtkCommand::ResetWindowLevelEvent, cbk);
      cbk->Delete();
    }

    this->Interactor->SetInteractorStyle(this->InteractorStyle);
    this->Interactor->SetRenderWindow(this->RenderWindow);
  }

  if (this->Renderer && this->ImageActor)
  {
    this->Renderer->AddViewProp(this->ImageActor);
  }

  if (this->ImageActor && this->WindowLevel)
  {
    this->ImageActor->GetMapper()->SetInputConnection(this->WindowLevel->GetOutputPort());
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer2::UnInstallPipeline()
{
  if (this->ImageActor)
  {
    this->ImageActor->GetMapper()->SetInputConnection(nullptr);
  }

  if (this->Renderer && this->ImageActor)
  {
    this->Renderer->RemoveViewProp(this->ImageActor);
  }

  if (this->RenderWindow && this->Renderer)
  {
    this->RenderWindow->RemoveRenderer(this->Renderer);
  }

  if (this->Interactor)
  {
    this->Interactor->SetInteractorStyle(nullptr);
    this->Interactor->SetRenderWindow(nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkImageViewer2::Render()
{
  if (this->FirstRender)
  {
    // Initialize the size if not set yet

    svtkAlgorithm* input = this->GetInputAlgorithm();
    if (input)
    {
      input->UpdateInformation();
      int* w_ext =
        this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      int xs = 0, ys = 0;

      switch (this->SliceOrientation)
      {
        case svtkImageViewer2::SLICE_ORIENTATION_XY:
        default:
          xs = w_ext[1] - w_ext[0] + 1;
          ys = w_ext[3] - w_ext[2] + 1;
          break;

        case svtkImageViewer2::SLICE_ORIENTATION_XZ:
          xs = w_ext[1] - w_ext[0] + 1;
          ys = w_ext[5] - w_ext[4] + 1;
          break;

        case svtkImageViewer2::SLICE_ORIENTATION_YZ:
          xs = w_ext[3] - w_ext[2] + 1;
          ys = w_ext[5] - w_ext[4] + 1;
          break;
      }

      // if it would be smaller than 150 by 100 then limit to 150 by 100
      if (this->RenderWindow->GetSize()[0] == 0)
      {
        this->RenderWindow->SetSize(xs < 150 ? 150 : xs, ys < 100 ? 100 : ys);
      }

      if (this->Renderer)
      {
        this->Renderer->ResetCamera();
        this->Renderer->GetActiveCamera()->SetParallelScale(xs < 150 ? 75 : (xs - 1) / 2.0);
      }
      this->FirstRender = 0;
    }
  }
  if (this->GetInput())
  {
    this->RenderWindow->Render();
  }
}

//----------------------------------------------------------------------------
const char* svtkImageViewer2::GetWindowName()
{
  return this->RenderWindow->GetWindowName();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetOffScreenRendering(svtkTypeBool i)
{
  this->RenderWindow->SetOffScreenRendering(i);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageViewer2::GetOffScreenRendering()
{
  return this->RenderWindow->GetOffScreenRendering();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetInputData(svtkImageData* in)
{
  this->WindowLevel->SetInputData(in);
  this->UpdateDisplayExtent();
}
//----------------------------------------------------------------------------
svtkImageData* svtkImageViewer2::GetInput()
{
  return svtkImageData::SafeDownCast(this->WindowLevel->GetInput());
}
//----------------------------------------------------------------------------
svtkInformation* svtkImageViewer2::GetInputInformation()
{
  return this->WindowLevel->GetInputInformation();
}
//----------------------------------------------------------------------------
svtkAlgorithm* svtkImageViewer2::GetInputAlgorithm()
{
  return this->WindowLevel->GetInputAlgorithm();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::SetInputConnection(svtkAlgorithmOutput* input)
{
  this->WindowLevel->SetInputConnection(input);
  this->UpdateDisplayExtent();
}

//----------------------------------------------------------------------------
void svtkImageViewer2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderWindow:\n";
  this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Renderer:\n";
  this->Renderer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ImageActor:\n";
  this->ImageActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "WindowLevel:\n" << endl;
  this->WindowLevel->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Slice: " << this->Slice << endl;
  os << indent << "SliceOrientation: " << this->SliceOrientation << endl;
  os << indent << "InteractorStyle: " << endl;
  if (this->InteractorStyle)
  {
    os << "\n";
    this->InteractorStyle->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "None";
  }
}
