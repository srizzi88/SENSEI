/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTextRepresentation.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkWindow.h"

class svtkTextRepresentationObserver : public svtkCommand
{
public:
  static svtkTextRepresentationObserver* New() { return new svtkTextRepresentationObserver; }

  void SetTarget(svtkTextRepresentation* t) { this->Target = t; }
  void Execute(svtkObject* o, unsigned long event, void* p) override
  {
    if (this->Target)
    {
      if (o && svtkTextActor::SafeDownCast(o))
      {
        this->Target->ExecuteTextActorModifiedEvent(o, event, p);
      }
      else if (o && svtkTextProperty::SafeDownCast(o))
      {
        this->Target->ExecuteTextPropertyModifiedEvent(o, event, p);
      }
    }
  }

protected:
  svtkTextRepresentationObserver() { this->Target = nullptr; }
  svtkTextRepresentation* Target;
};

svtkStandardNewMacro(svtkTextRepresentation);

//-------------------------------------------------------------------------
svtkTextRepresentation::svtkTextRepresentation()
{
  this->Observer = svtkTextRepresentationObserver::New();
  this->Observer->SetTarget(this);

  this->TextActor = svtkTextActor::New();
  this->InitializeTextActor();

  this->SetShowBorder(svtkBorderRepresentation::BORDER_ACTIVE);
  this->BWActor->VisibilityOff();
  this->WindowLocation = AnyLocation;
}

//-------------------------------------------------------------------------
svtkTextRepresentation::~svtkTextRepresentation()
{
  this->SetTextActor(nullptr);
  this->Observer->SetTarget(nullptr);
  this->Observer->Delete();
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::SetTextActor(svtkTextActor* textActor)
{
  if (textActor != this->TextActor)
  {
    if (this->TextActor)
    {
      this->TextActor->GetTextProperty()->RemoveObserver(this->Observer);
      this->TextActor->RemoveObserver(this->Observer);
      this->TextActor->Delete();
    }
    this->TextActor = textActor;
    if (this->TextActor)
    {
      this->TextActor->Register(this);
    }

    this->InitializeTextActor();
    this->Modified();
  }
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::SetText(const char* text)
{
  if (this->TextActor)
  {
    this->TextActor->SetInput(text);
  }
  else
  {
    svtkErrorMacro("No Text Actor present. Cannot set text.");
  }
}

//-------------------------------------------------------------------------
const char* svtkTextRepresentation::GetText()
{
  if (this->TextActor)
  {
    return this->TextActor->GetInput();
  }
  svtkErrorMacro("No text actor present. No showing any text.");
  return nullptr;
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::BuildRepresentation()
{
  // Ask the superclass the size and set the text
  int* pos1 = this->PositionCoordinate->GetComputedDisplayValue(this->Renderer);
  int* pos2 = this->Position2Coordinate->GetComputedDisplayValue(this->Renderer);

  if (this->TextActor)
  {
    this->TextActor->GetPositionCoordinate()->SetValue(pos1[0], pos1[1]);
    this->TextActor->GetPosition2Coordinate()->SetValue(pos2[0], pos2[1]);
  }

  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::GetActors2D(svtkPropCollection* pc)
{
  pc->AddItem(this->TextActor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->TextActor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int svtkTextRepresentation::RenderOverlay(svtkViewport* w)
{
  int count = this->Superclass::RenderOverlay(w);
  count += this->TextActor->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int svtkTextRepresentation::RenderOpaqueGeometry(svtkViewport* w)
{
  // CheckTextBoundary resize the text actor. This needs to happen before we
  // actually render (previous version was calling this method after
  // this->TextActor->RenderOpaqueGeometry(), which seems like a bug).
  this->CheckTextBoundary();
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->TextActor->RenderOpaqueGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
int svtkTextRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  count += this->TextActor->RenderTranslucentPolygonalGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
svtkTypeBool svtkTextRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  result |= this->TextActor->HasTranslucentPolygonalGeometry();
  return result;
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::InitializeTextActor()
{
  if (this->TextActor)
  {
    this->TextActor->SetTextScaleModeToProp();
    this->TextActor->SetMinimumSize(1, 1);
    this->TextActor->SetMaximumLineHeight(1.0);
    this->TextActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
    this->TextActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
    this->TextActor->GetPosition2Coordinate()->SetReferenceCoordinate(nullptr);
    this->TextActor->GetTextProperty()->SetJustificationToCentered();
    this->TextActor->GetTextProperty()->SetVerticalJustificationToCentered();

    this->TextActor->UseBorderAlignOn();

    this->TextProperty = this->TextActor->GetTextProperty();

    this->TextActor->GetTextProperty()->AddObserver(svtkCommand::ModifiedEvent, this->Observer);
    this->TextActor->AddObserver(svtkCommand::ModifiedEvent, this->Observer);
  }
}

//----------------------------------------------------------------------------
void svtkTextRepresentation::ExecuteTextPropertyModifiedEvent(
  svtkObject* object, unsigned long enumEvent, void*)
{
  if (!object || enumEvent != svtkCommand::ModifiedEvent)
  {
    return;
  }
  svtkTextProperty* tp = svtkTextProperty::SafeDownCast(object);
  if (!tp)
  {
    return;
  }

  this->CheckTextBoundary();
}

//----------------------------------------------------------------------------
void svtkTextRepresentation::ExecuteTextActorModifiedEvent(
  svtkObject* object, unsigned long enumEvent, void*)
{
  if (!object || enumEvent != svtkCommand::ModifiedEvent)
  {
    return;
  }
  svtkTextActor* ta = svtkTextActor::SafeDownCast(object);
  if (!ta || ta != this->TextActor)
  {
    return;
  }

  if (this->TextProperty != this->TextActor->GetTextProperty())
  {
    this->TextActor->GetTextProperty()->AddObserver(svtkCommand::ModifiedEvent, this->Observer);
    this->TextProperty = this->TextActor->GetTextProperty();
  }

  this->CheckTextBoundary();
}

//----------------------------------------------------------------------------
void svtkTextRepresentation::CheckTextBoundary()
{
  if (this->GetRenderer() &&
    this->TextActor->GetTextScaleMode() != svtkTextActor::TEXT_SCALE_MODE_PROP)
  {
    svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
    if (!tren)
    {
      svtkErrorMacro(<< "Failed getting the svtkTextRenderer instance");
      return;
    }

    this->TextActor->ComputeScaledFont(this->GetRenderer());

    svtkWindow* win = this->Renderer->GetSVTKWindow();
    if (!win)
    {
      svtkErrorMacro(<< "No render window available: cannot determine DPI.");
      return;
    }

    int text_bbox[4];
    if (!tren->GetBoundingBox(
          this->TextActor->GetScaledTextProperty(), this->GetText(), text_bbox, win->GetDPI()))
    {
      return;
    }

    // The bounding box was the area that is going to be filled with pixels
    // given a text origin of (0, 0). Now get the real size we need, i.e.
    // the full extent from the origin to the bounding box.

    double text_size[2];
    text_size[0] = (text_bbox[1] - text_bbox[0] + 1);
    text_size[1] = (text_bbox[3] - text_bbox[2] + 1);

    this->GetRenderer()->DisplayToNormalizedDisplay(text_size[0], text_size[1]);
    this->GetRenderer()->NormalizedDisplayToViewport(text_size[0], text_size[1]);
    this->GetRenderer()->ViewportToNormalizedViewport(text_size[0], text_size[1]);

    // update the PositionCoordinate

    double* pos2 = this->Position2Coordinate->GetValue();
    if (pos2[0] != text_size[0] || pos2[1] != text_size[1])
    {
      this->Position2Coordinate->SetValue(text_size[0], text_size[1], 0);
      this->Modified();
    }
    if (this->WindowLocation != AnyLocation)
    {
      this->UpdateWindowLocation();
    }
  }
}

//----------------------------------------------------------------------------
void svtkTextRepresentation::SetWindowLocation(int enumLocation)
{
  if (this->WindowLocation == enumLocation)
  {
    return;
  }

  this->WindowLocation = enumLocation;
  this->CheckTextBoundary();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkTextRepresentation::SetPosition(double x, double y)
{
  double* pos = this->PositionCoordinate->GetValue();
  if (pos[0] == x && pos[1] == y)
  {
    return;
  }

  this->PositionCoordinate->SetValue(x, y);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkTextRepresentation::UpdateWindowLocation()
{
  if (this->WindowLocation != AnyLocation)
  {
    double* pos2 = this->Position2Coordinate->GetValue();
    switch (this->WindowLocation)
    {
      case LowerLeftCorner:
        this->SetPosition(0.01, 0.01);
        break;
      case LowerRightCorner:
        this->SetPosition(0.99 - pos2[0], 0.01);
        break;
      case LowerCenter:
        this->SetPosition((1 - pos2[0]) / 2.0, 0.01);
        break;
      case UpperLeftCorner:
        this->SetPosition(0.01, 0.99 - pos2[1]);
        break;
      case UpperRightCorner:
        this->SetPosition(0.99 - pos2[0], 0.99 - pos2[1]);
        break;
      case UpperCenter:
        this->SetPosition((1 - pos2[0]) / 2.0, 0.99 - pos2[1]);
        break;
      default:
        break;
    }
  }
}

//-------------------------------------------------------------------------
void svtkTextRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Text Actor: " << this->TextActor << "\n";

  os << indent << "Window Location: ";
  switch (this->WindowLocation)
  {
    case LowerLeftCorner:
      os << "LowerLeftCorner\n";
      break;
    case LowerRightCorner:
      os << "LowerRightCorner\n";
      break;
    case LowerCenter:
      os << "LowerCenter\n";
      break;
    case UpperLeftCorner:
      os << "UpperLeftCorner\n";
      break;
    case UpperRightCorner:
      os << "UpperRightCorner\n";
      break;
    case UpperCenter:
      os << "UpperCenter\n";
      break;
  }
}
