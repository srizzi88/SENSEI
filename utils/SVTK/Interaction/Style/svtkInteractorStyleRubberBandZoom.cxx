/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleRubberBandZoom.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleRubberBandZoom.h"

#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVectorOperators.h"

namespace impl
{
template <class T>
svtkVector3d GetCenter(const svtkRect<T>& rect)
{
  return svtkVector3d(
    rect.GetX() + 0.5 * rect.GetWidth(), rect.GetY() + 0.5 * rect.GetHeight(), 0.0);
}

svtkVector3d DisplayToWorld(const svtkVector3d& display, svtkRenderer* ren)
{
  ren->SetDisplayPoint(display[0], display[1], display[2]);
  ren->DisplayToView();
  ren->ViewToWorld();

  svtkVector<double, 4> world4;
  ren->GetWorldPoint(world4.GetData());
  double invw = 1.0 * world4[3];
  world4 = world4 * invw;
  return svtkVector3d(world4.GetData());
}
}

svtkStandardNewMacro(svtkInteractorStyleRubberBandZoom);

svtkInteractorStyleRubberBandZoom::svtkInteractorStyleRubberBandZoom()
{
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->Moving = 0;
  this->LockAspectToViewport = false;
  this->CenterAtStartPosition = false;
  this->UseDollyForPerspectiveProjection = true;
  this->PixelArray = svtkUnsignedCharArray::New();
}

svtkInteractorStyleRubberBandZoom::~svtkInteractorStyleRubberBandZoom()
{
  this->PixelArray->Delete();
}

void svtkInteractorStyleRubberBandZoom::AdjustBox(int startPosition[2], int endPosition[2]) const
{
  if (this->LockAspectToViewport && this->CurrentRenderer != nullptr)
  {
    double aspect = this->CurrentRenderer->GetAspect()[0];

    int dx = endPosition[0] - startPosition[0];
    int dy = endPosition[1] - startPosition[1];

    // now we could adjust dx or dy. We pick one that ensures that the current end
    // position is always included in the new box. That way, the mouse is not
    // floating outside of the box.
    const int newDY = static_cast<int>(0.5 + std::abs(dx) / aspect);
    if (std::abs(dy) > newDY)
    {
      const int newDX = static_cast<int>(0.5 + aspect * std::abs(dy));
      dx = (dx < 0) ? -newDX : newDX;
    }
    else
    {
      dy = (dy < 0) ? -newDY : newDY;
    }
    endPosition[0] = startPosition[0] + dx;
    endPosition[1] = startPosition[1] + dy;
  }

  bool centerAtStartPosition = this->CenterAtStartPosition;
  if (this->Interactor && (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey()))
  {
    centerAtStartPosition = !centerAtStartPosition;
  }
  if (centerAtStartPosition)
  {
    svtkVector2i dia;
    dia = svtkVector2i(endPosition) - svtkVector2i(startPosition);
    svtkVector2i newStartPosition = svtkVector2i(startPosition) - dia;
    startPosition[0] = newStartPosition.GetX();
    startPosition[1] = newStartPosition.GetY();
  }
}

void svtkInteractorStyleRubberBandZoom::OnMouseMove()
{
  if (!this->Interactor || !this->Moving)
  {
    return;
  }

  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];
  const int* size = this->Interactor->GetRenderWindow()->GetSize();
  if (this->EndPosition[0] > (size[0] - 1))
  {
    this->EndPosition[0] = size[0] - 1;
  }
  if (this->EndPosition[0] < 0)
  {
    this->EndPosition[0] = 0;
  }
  if (this->EndPosition[1] > (size[1] - 1))
  {
    this->EndPosition[1] = size[1] - 1;
  }
  if (this->EndPosition[1] < 0)
  {
    this->EndPosition[1] = 0;
  }

  int startPosition[2] = { this->StartPosition[0], this->StartPosition[1] };
  int endPosition[2] = { this->EndPosition[0], this->EndPosition[1] };
  // Adjust box to fit aspect ratio, if needed.
  this->AdjustBox(startPosition, endPosition);

  svtkUnsignedCharArray* tmpPixelArray = svtkUnsignedCharArray::New();
  tmpPixelArray->DeepCopy(this->PixelArray);

  unsigned char* pixels = tmpPixelArray->GetPointer(0);

  int minX = svtkMath::Min(startPosition[0], endPosition[0]);
  int minY = svtkMath::Min(startPosition[1], endPosition[1]);
  int maxX = svtkMath::Max(startPosition[0], endPosition[0]);
  int maxY = svtkMath::Max(startPosition[1], endPosition[1]);

  int clampedMinX = svtkMath::Max(minX, 0);
  int clampedMaxX = svtkMath::Min(maxX, size[0] - 1);
  int clampedMinY = svtkMath::Max(minY, 0);
  int clampedMaxY = svtkMath::Min(maxY, size[1] - 1);

  // Draw zoom box
  // Draw bottom horizontal line
  if (minY >= 0 && minY < size[1])
  {
    for (int i = clampedMinX; i < clampedMaxX; ++i)
    {
      pixels[3 * (minY * size[0] + i)] = 255 ^ pixels[3 * (minY * size[0] + i)];
      pixels[3 * (minY * size[0] + i) + 1] = 255 ^ pixels[3 * (minY * size[0] + i) + 1];
      pixels[3 * (minY * size[0] + i) + 2] = 255 ^ pixels[3 * (minY * size[0] + i) + 2];
    }
  }

  // Draw top horizontal line
  if (maxY >= 0 && maxY < size[1])
  {
    for (int i = clampedMinX; i < clampedMaxX; ++i)
    {
      pixels[3 * (maxY * size[0] + i)] = 255 ^ pixels[3 * (maxY * size[0] + i)];
      pixels[3 * (maxY * size[0] + i) + 1] = 255 ^ pixels[3 * (maxY * size[0] + i) + 1];
      pixels[3 * (maxY * size[0] + i) + 2] = 255 ^ pixels[3 * (maxY * size[0] + i) + 2];
    }
  }

  // Draw left vertical line
  if (minX >= 0 && minX < size[0])
  {
    for (int i = clampedMinY; i < clampedMaxY; ++i)
    {
      pixels[3 * (i * size[0] + minX)] = 255 ^ pixels[3 * (i * size[0] + minX)];
      pixels[3 * (i * size[0] + minX) + 1] = 255 ^ pixels[3 * (i * size[0] + minX) + 1];
      pixels[3 * (i * size[0] + minX) + 2] = 255 ^ pixels[3 * (i * size[0] + minX) + 2];
    }
  }

  // Draw right vertical line
  if (maxX >= 0 && maxX < size[0])
  {
    for (int i = clampedMinY; i < clampedMaxY; ++i)
    {
      pixels[3 * (i * size[0] + maxX)] = 255 ^ pixels[3 * (i * size[0] + maxX)];
      pixels[3 * (i * size[0] + maxX) + 1] = 255 ^ pixels[3 * (i * size[0] + maxX) + 1];
      pixels[3 * (i * size[0] + maxX) + 2] = 255 ^ pixels[3 * (i * size[0] + maxX) + 2];
    }
  }

  this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 0);
  this->Interactor->GetRenderWindow()->Frame();

  tmpPixelArray->Delete();
}

void svtkInteractorStyleRubberBandZoom::OnLeftButtonDown()
{
  if (!this->Interactor)
  {
    return;
  }
  this->Moving = 1;

  svtkRenderWindow* renWin = this->Interactor->GetRenderWindow();

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];

  this->PixelArray->Initialize();
  this->PixelArray->SetNumberOfComponents(3);
  const int* size = renWin->GetSize();
  this->PixelArray->SetNumberOfTuples(size[0] * size[1]);

  renWin->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 1, this->PixelArray);

  this->FindPokedRenderer(this->StartPosition[0], this->StartPosition[1]);
  if (this->CurrentRenderer)
  {
    // Ensure the aspect ratio is up-to-date.
    this->CurrentRenderer->ComputeAspect();
  }
}

void svtkInteractorStyleRubberBandZoom::OnLeftButtonUp()
{
  if (!this->Interactor || !this->Moving)
  {
    return;
  }

  if ((this->StartPosition[0] != this->EndPosition[0]) ||
    (this->StartPosition[1] != this->EndPosition[1]))
  {
    this->Zoom();
  }
  this->Moving = 0;
}

void svtkInteractorStyleRubberBandZoom::Zoom()
{
  int startPosition[2] = { this->StartPosition[0], this->StartPosition[1] };
  int endPosition[2] = { this->EndPosition[0], this->EndPosition[1] };

  // Adjust box to fit aspect ratio, if needed.
  this->AdjustBox(startPosition, endPosition);

  const svtkRecti box(startPosition[0] < endPosition[0] ? startPosition[0] : endPosition[0],
    startPosition[1] < endPosition[1] ? startPosition[1] : endPosition[1],
    std::abs(endPosition[0] - startPosition[0]), std::abs(endPosition[1] - startPosition[1]));

  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  if (cam->GetParallelProjection() || this->UseDollyForPerspectiveProjection)
  {
    this->ZoomTraditional(box);
  }
  else
  {
    this->ZoomPerspectiveProjectionUsingViewAngle(box);
  }
  this->Interactor->Render();
}

void svtkInteractorStyleRubberBandZoom::ZoomTraditional(const svtkRecti& box)
{
  const int* size = this->CurrentRenderer->GetSize();
  const int* origin = this->CurrentRenderer->GetOrigin();
  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();

  const svtkVector3d rbcenter = impl::GetCenter(box);
  const svtkVector3d worldRBCenter = impl::DisplayToWorld(rbcenter, this->CurrentRenderer);

  const svtkVector3d winCenter = impl::GetCenter(svtkRecti(origin[0], origin[1], size[0], size[1]));
  const svtkVector3d worldWinCenter = impl::DisplayToWorld(winCenter, this->CurrentRenderer);
  const svtkVector3d translation = worldRBCenter - worldWinCenter;

  svtkVector3d pos, fp;
  cam->GetPosition(pos.GetData());
  cam->GetFocalPoint(fp.GetData());

  pos = pos + translation;
  fp = fp + translation;

  cam->SetPosition(pos.GetData());
  cam->SetFocalPoint(fp.GetData());

  double zoomFactor;
  if (box.GetWidth() > box.GetHeight())
  {
    zoomFactor = size[0] / static_cast<double>(box.GetWidth());
  }
  else
  {
    zoomFactor = size[1] / static_cast<double>(box.GetHeight());
  }

  if (cam->GetParallelProjection())
  {
    cam->Zoom(zoomFactor);
  }
  else
  {
    // In perspective mode, zoom in by moving the camera closer.  Because we are
    // moving the camera closer, we have to be careful to try to adjust the
    // clipping planes to best match the actual position they were in before.
    double initialDistance = cam->GetDistance();
    cam->Dolly(zoomFactor);

    double finalDistance = cam->GetDistance();
    double deltaDistance = initialDistance - finalDistance;
    double clippingRange[2];
    cam->GetClippingRange(clippingRange);
    clippingRange[0] -= deltaDistance;
    clippingRange[1] -= deltaDistance;
    // Correct bringing clipping planes too close or behind camera.
    if (clippingRange[1] <= 0.0)
    {
      clippingRange[1] = 0.001;
    }
    // This near plane check comes from svtkRenderer::ResetCameraClippingRange()
    if (clippingRange[0] < 0.001 * clippingRange[1])
    {
      clippingRange[0] = 0.001 * clippingRange[1];
    }
    cam->SetClippingRange(clippingRange);
  }
}

void svtkInteractorStyleRubberBandZoom::ZoomPerspectiveProjectionUsingViewAngle(const svtkRecti& box)
{
  const int* size = this->CurrentRenderer->GetSize();
  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();

  const svtkVector3d rbcenter = impl::GetCenter(box);
  const svtkVector3d worldRBCenter = impl::DisplayToWorld(rbcenter, this->CurrentRenderer);
  cam->SetFocalPoint(worldRBCenter.GetData());

  double zoomFactor;
  if (box.GetWidth() > box.GetHeight())
  {
    zoomFactor = size[0] / static_cast<double>(box.GetWidth());
  }
  else
  {
    zoomFactor = size[1] / static_cast<double>(box.GetHeight());
  }

  cam->Zoom(zoomFactor);
}

void svtkInteractorStyleRubberBandZoom::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LockAspectToViewport: " << this->LockAspectToViewport << endl;
  os << indent << "CenterAtStartPosition: " << this->CenterAtStartPosition << endl;
  os << indent << "UseDollyForPerspectiveProjection: " << this->UseDollyForPerspectiveProjection
     << endl;
}
