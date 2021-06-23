/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor2D.h"

#include "svtkMapper2D.h"
#include "svtkObjectFactory.h"
#include "svtkPropCollection.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkActor2D);

svtkCxxSetObjectMacro(svtkActor2D, Property, svtkProperty2D);
svtkCxxSetObjectMacro(svtkActor2D, Mapper, svtkMapper2D);

//----------------------------------------------------------------------------
// Creates an actor2D with the following defaults:
// position -1, -1 (view coordinates)
// orientation 0, scale (1,1), layer 0, visibility on
svtkActor2D::svtkActor2D()
{
  this->Mapper = nullptr;
  this->LayerNumber = 0;
  this->Property = nullptr;
  //
  this->PositionCoordinate = svtkCoordinate::New();
  this->PositionCoordinate->SetCoordinateSystem(SVTK_VIEWPORT);
  //
  this->Position2Coordinate = svtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.5, 0.5);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);
}

//----------------------------------------------------------------------------
// Destroy an actor2D.
svtkActor2D::~svtkActor2D()
{
  if (this->Property)
  {
    this->Property->UnRegister(this);
    this->Property = nullptr;
  }
  if (this->PositionCoordinate)
  {
    this->PositionCoordinate->Delete();
    this->PositionCoordinate = nullptr;
  }
  if (this->Position2Coordinate)
  {
    this->Position2Coordinate->Delete();
    this->Position2Coordinate = nullptr;
  }
  if (this->Mapper != nullptr)
  {
    this->Mapper->UnRegister(this);
    this->Mapper = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkActor2D::ReleaseGraphicsResources(svtkWindow* win)
{
  // pass this information onto the mapper
  if (this->Mapper)
  {
    this->Mapper->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int svtkActor2D::RenderOverlay(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkActor2D::RenderOverlay");

  // Is the viewport's RenderWindow capturing GL2PS-special prop, and does this
  // actor represent text or mathtext?
  if (svtkRenderer* renderer = svtkRenderer::SafeDownCast(viewport))
  {
    if (svtkRenderWindow* renderWindow = renderer->GetRenderWindow())
    {
      if (renderWindow->GetCapturingGL2PSSpecialProps())
      {
        if (this->IsA("svtkTextActor") || this->IsA("svtkTexturedActor2D") ||
          (this->Mapper &&
            (this->Mapper->IsA("svtkTextMapper") || this->Mapper->IsA("svtkLabeledDataMapper"))))
        {
          renderer->CaptureGL2PSSpecialProp(this);
        }
      }
    }
  }

  if (!this->Property)
  {
    svtkDebugMacro(<< "svtkActor2D::Render - Creating Property2D");
    // Force creation of default property
    this->GetProperty();
  }

  this->Property->Render(viewport);

  if (!this->Mapper)
  {
    svtkErrorMacro(<< "svtkActor2D::Render - No mapper set");
    return 0;
  }

  this->Mapper->RenderOverlay(viewport, this);

  return 1;
}

//----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int svtkActor2D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkActor2D::RenderOpaqueGeometry");

  if (!this->Property)
  {
    svtkDebugMacro(<< "svtkActor2D::Render - Creating Property2D");
    // Force creation of default property
    this->GetProperty();
  }

  this->Property->Render(viewport);

  if (!this->Mapper)
  {
    svtkErrorMacro(<< "svtkActor2D::Render - No mapper set");
    return 0;
  }

  this->Mapper->RenderOpaqueGeometry(viewport, this);

  return 1;
}

//-----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int svtkActor2D::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkActor2D::RenderTranslucentPolygonalGeometry");

  if (!this->Property)
  {
    svtkDebugMacro(<< "svtkActor2D::Render - Creating Property2D");
    // Force creation of default property
    this->GetProperty();
  }

  this->Property->Render(viewport);

  if (!this->Mapper)
  {
    svtkErrorMacro(<< "svtkActor2D::Render - No mapper set");
    return 0;
  }

  this->Mapper->RenderTranslucentPolygonalGeometry(viewport, this);

  return 1;
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkActor2D::HasTranslucentPolygonalGeometry()
{
  int result;
  if (this->Mapper)
  {
    result = this->Mapper->HasTranslucentPolygonalGeometry();
  }
  else
  {
    svtkErrorMacro(<< "svtkActor2D::HasTranslucentPolygonalGeometry - No mapper set");
    result = 0;
  }
  return result;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkActor2D::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  time = this->PositionCoordinate->GetMTime();
  mTime = (time > mTime ? time : mTime);
  time = this->Position2Coordinate->GetMTime();
  mTime = (time > mTime ? time : mTime);

  if (this->Property != nullptr)
  {
    time = this->Property->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
// Set the Prop2D's position in display coordinates.
void svtkActor2D::SetDisplayPosition(int XPos, int YPos)
{
  this->PositionCoordinate->SetCoordinateSystem(SVTK_DISPLAY);
  this->PositionCoordinate->SetValue(static_cast<float>(XPos), static_cast<float>(YPos), 0.0);
}

//----------------------------------------------------------------------------
void svtkActor2D::SetWidth(double w)
{
  double* pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(w, pos[1]);
}

//----------------------------------------------------------------------------
void svtkActor2D::SetHeight(double w)
{
  double* pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(pos[0], w);
}

//----------------------------------------------------------------------------
double svtkActor2D::GetWidth()
{
  return this->Position2Coordinate->GetValue()[0];
}

//----------------------------------------------------------------------------
double svtkActor2D::GetHeight()
{
  return this->Position2Coordinate->GetValue()[1];
}

//----------------------------------------------------------------------------
// Returns an Prop2D's property2D.  Creates a property if one
// doesn't already exist.
svtkProperty2D* svtkActor2D::GetProperty()
{
  if (this->Property == nullptr)
  {
    this->Property = svtkProperty2D::New();
    this->Property->Register(this);
    this->Property->Delete();
    this->Modified();
  }
  return this->Property;
}

//----------------------------------------------------------------------------
void svtkActor2D::GetActors2D(svtkPropCollection* ac)
{
  ac->AddItem(this);
}

//----------------------------------------------------------------------------
void svtkActor2D::ShallowCopy(svtkProp* prop)
{
  svtkActor2D* a = svtkActor2D::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetMapper(a->GetMapper());
    this->SetLayerNumber(a->GetLayerNumber());
    this->SetProperty(a->GetProperty());
    this->SetPosition(a->GetPosition());
    this->SetPosition2(a->GetPosition2());
  }

  // Now do superclass
  this->svtkProp::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void svtkActor2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Layer Number: " << this->LayerNumber << "\n";
  os << indent << "PositionCoordinate: " << this->PositionCoordinate << "\n";
  this->PositionCoordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Position2 Coordinate: " << this->Position2Coordinate << "\n";
  this->Position2Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Property: " << this->Property << "\n";
  if (this->Property)
  {
    this->Property->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Mapper: " << this->Mapper << "\n";
  if (this->Mapper)
  {
    this->Mapper->PrintSelf(os, indent.GetNextIndent());
  }
}
