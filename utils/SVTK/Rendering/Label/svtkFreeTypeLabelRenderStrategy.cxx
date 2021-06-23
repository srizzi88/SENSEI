/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFreeTypeLabelRenderStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFreeTypeLabelRenderStrategy.h"

#include "svtkActor2D.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkTimerLog.h"
#include "svtkWindow.h"

svtkStandardNewMacro(svtkFreeTypeLabelRenderStrategy);

//----------------------------------------------------------------------------
svtkFreeTypeLabelRenderStrategy::svtkFreeTypeLabelRenderStrategy()
{
  this->TextRenderer = svtkTextRenderer::GetInstance();
  this->Mapper = svtkTextMapper::New();
  this->Actor = svtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
svtkFreeTypeLabelRenderStrategy::~svtkFreeTypeLabelRenderStrategy()
{
  this->Mapper->Delete();
  this->Actor->Delete();
}

void svtkFreeTypeLabelRenderStrategy::ReleaseGraphicsResources(svtkWindow* window)
{
  this->Actor->ReleaseGraphicsResources(window);
}

// double compute_bounds_time1 = 0;
// int compute_bounds_iter1 = 0;
//----------------------------------------------------------------------------
void svtkFreeTypeLabelRenderStrategy::ComputeLabelBounds(
  svtkTextProperty* tprop, svtkUnicodeString label, double bds[4])
{
  // svtkTimerLog* timer = svtkTimerLog::New();
  // timer->StartTimer();

  // Check for empty string.
  svtkStdString str;
  label.utf8_str(str);
  if (str.length() == 0)
  {
    bds[0] = 0;
    bds[1] = 0;
    bds[2] = 0;
    bds[3] = 0;
    return;
  }

  if (!tprop)
  {
    tprop = this->DefaultTextProperty;
  }
  svtkSmartPointer<svtkTextProperty> copy = tprop;
  if (tprop->GetOrientation() != 0.0)
  {
    copy = svtkSmartPointer<svtkTextProperty>::New();
    copy->ShallowCopy(tprop);
    copy->SetOrientation(0.0);
  }

  int dpi = 72;
  if (this->Renderer && this->Renderer->GetSVTKWindow())
  {
    dpi = this->Renderer->GetSVTKWindow()->GetDPI();
  }
  else
  {
    svtkWarningMacro(<< "No Renderer set. Assuming DPI of " << dpi << ".");
  }

  int bbox[4];
  this->TextRenderer->GetBoundingBox(copy, label.utf8_str(), bbox, dpi);

  // Take line offset into account
  bds[0] = bbox[0];
  bds[1] = bbox[1];
  bds[2] = bbox[2] - tprop->GetLineOffset();
  bds[3] = bbox[3] - tprop->GetLineOffset();

  // Take justification into account
  double sz[2] = { bds[1] - bds[0], bds[3] - bds[2] };
  switch (tprop->GetJustification())
  {
    case SVTK_TEXT_LEFT:
      break;
    case SVTK_TEXT_CENTERED:
      bds[0] -= sz[0] / 2;
      bds[1] -= sz[0] / 2;
      break;
    case SVTK_TEXT_RIGHT:
      bds[0] -= sz[0];
      bds[1] -= sz[0];
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    case SVTK_TEXT_BOTTOM:
      break;
    case SVTK_TEXT_CENTERED:
      bds[2] -= sz[1] / 2;
      bds[3] -= sz[1] / 2;
      break;
    case SVTK_TEXT_TOP:
      bds[2] -= sz[1];
      bds[3] -= sz[1];
      break;
  }
  // timer->StopTimer();
  // compute_bounds_time1 += timer->GetElapsedTime();
  // compute_bounds_iter1++;
  // if (compute_bounds_iter1 % 10000 == 0)
  //  {
  //  cerr << "ComputeLabelBounds time: " << (compute_bounds_time1 / compute_bounds_iter1) << endl;
  //  }
}

// double render_label_time1 = 0;
// int render_label_iter1 = 0;
//----------------------------------------------------------------------------
void svtkFreeTypeLabelRenderStrategy::RenderLabel(
  int x[2], svtkTextProperty* tprop, svtkUnicodeString label)
{
  // svtkTimerLog* timer = svtkTimerLog::New();
  // timer->StartTimer();

  if (!this->Renderer)
  {
    svtkErrorMacro("Renderer must be set before rendering labels.");
    return;
  }
  if (!tprop)
  {
    tprop = this->DefaultTextProperty;
  }
  this->Mapper->SetTextProperty(tprop);
  this->Mapper->SetInput(label.utf8_str());
  this->Actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->Actor->GetPositionCoordinate()->SetValue(x[0], x[1], 0.0);
  this->Mapper->RenderOverlay(this->Renderer, this->Actor);
  // timer->StopTimer();
  // render_label_time1 += timer->GetElapsedTime();
  // render_label_iter1++;
  // if (render_label_iter1 % 100 == 0)
  //  {
  //  cerr << "RenderLabel time: " << (render_label_time1 / render_label_iter1) << endl;
  //  }
}

//----------------------------------------------------------------------------
void svtkFreeTypeLabelRenderStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
