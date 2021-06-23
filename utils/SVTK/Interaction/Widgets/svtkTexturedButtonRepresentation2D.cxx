/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedButtonRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTexturedButtonRepresentation2D.h"
#include "svtkBalloonRepresentation.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkImageData.h"
#include "svtkInteractorObserver.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include <map>

svtkStandardNewMacro(svtkTexturedButtonRepresentation2D);

svtkCxxSetObjectMacro(svtkTexturedButtonRepresentation2D, Property, svtkProperty2D);
svtkCxxSetObjectMacro(svtkTexturedButtonRepresentation2D, HoveringProperty, svtkProperty2D);
svtkCxxSetObjectMacro(svtkTexturedButtonRepresentation2D, SelectingProperty, svtkProperty2D);

// Map of textures
class svtkTextureArray : public std::map<int, svtkSmartPointer<svtkImageData> >
{
};
typedef std::map<int, svtkSmartPointer<svtkImageData> >::iterator svtkTextureArrayIterator;

//----------------------------------------------------------------------
svtkTexturedButtonRepresentation2D::svtkTexturedButtonRepresentation2D()
{
  // Configure the balloon
  this->Balloon = svtkBalloonRepresentation::New();
  this->Balloon->SetOffset(0, 0);

  // Set up the initial properties
  this->CreateDefaultProperties();

  // List of textures
  this->TextureArray = new svtkTextureArray;

  // Anchor point assuming that the button is anchored in 3D
  // If nullptr, then the placement occurs in display space
  this->Anchor = nullptr;
}

//----------------------------------------------------------------------
svtkTexturedButtonRepresentation2D::~svtkTexturedButtonRepresentation2D()
{
  this->Balloon->Delete();

  if (this->Property)
  {
    this->Property->Delete();
    this->Property = nullptr;
  }

  if (this->HoveringProperty)
  {
    this->HoveringProperty->Delete();
    this->HoveringProperty = nullptr;
  }

  if (this->SelectingProperty)
  {
    this->SelectingProperty->Delete();
    this->SelectingProperty = nullptr;
  }

  delete this->TextureArray;

  if (this->Anchor)
  {
    this->Anchor->Delete();
  }
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::SetButtonTexture(int i, svtkImageData* image)
{
  if (i < 0)
  {
    i = 0;
  }
  if (i >= this->NumberOfStates)
  {
    i = this->NumberOfStates - 1;
  }

  (*this->TextureArray)[i] = image;
}

//-------------------------------------------------------------------------
svtkImageData* svtkTexturedButtonRepresentation2D::GetButtonTexture(int i)
{
  if (i < 0)
  {
    i = 0;
  }
  if (i >= this->NumberOfStates)
  {
    i = this->NumberOfStates - 1;
  }

  svtkTextureArrayIterator iter = this->TextureArray->find(i);
  if (iter != this->TextureArray->end())
  {
    return (*iter).second;
  }
  else
  {
    return nullptr;
  }
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);
  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  if (this->Anchor)
  { // no longer in world space
    this->Anchor->Delete();
    this->Anchor = nullptr;
  }

  double e[2];
  e[0] = static_cast<double>(bounds[0]);
  e[1] = static_cast<double>(bounds[2]);
  this->Balloon->StartWidgetInteraction(e);
  this->Balloon->SetImageSize(
    static_cast<int>(bounds[1] - bounds[0]), static_cast<int>(bounds[3] - bounds[2]));
}

//-------------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::PlaceWidget(double anchor[3], int size[2])
{
  if (!this->Anchor)
  {
    this->Anchor = svtkCoordinate::New();
    this->Anchor->SetCoordinateSystemToWorld();
  }

  this->Anchor->SetValue(anchor);

  double e[2];
  e[0] = e[1] = 0.0;
  if (this->Renderer)
  {
    double* p = this->Anchor->GetComputedDoubleDisplayValue(this->Renderer);
    this->Balloon->SetRenderer(this->Renderer);
    this->Balloon->StartWidgetInteraction(p);
    e[0] = static_cast<double>(p[0]);
    e[1] = static_cast<double>(p[1]);
  }
  else
  {
    this->Balloon->StartWidgetInteraction(e);
  }

  this->Balloon->SetImageSize(size);

  this->InitialBounds[0] = e[0];
  this->InitialBounds[1] = e[0] + size[0];
  this->InitialBounds[2] = e[1];
  this->InitialBounds[3] = e[1] + size[1];
  this->InitialBounds[4] = this->InitialBounds[5] = 0.0;

  double* bounds = this->InitialBounds;
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
}

//-------------------------------------------------------------------------
int svtkTexturedButtonRepresentation2D ::ComputeInteractionState(
  int X, int Y, int svtkNotUsed(modify))
{
  this->Balloon->SetRenderer(this->GetRenderer());
  if (this->Balloon->ComputeInteractionState(X, Y) == svtkBalloonRepresentation::OnImage)
  {
    this->InteractionState = svtkButtonRepresentation::Inside;
  }
  else
  {
    this->InteractionState = svtkButtonRepresentation::Outside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::Highlight(int highlight)
{
  this->Superclass::Highlight(highlight);

  svtkProperty2D* initialProperty = this->Balloon->GetImageProperty();
  svtkProperty2D* selectedProperty;

  if (highlight == svtkButtonRepresentation::HighlightHovering)
  {
    this->Balloon->SetImageProperty(this->HoveringProperty);
    selectedProperty = this->HoveringProperty;
  }
  else if (highlight == svtkButtonRepresentation::HighlightSelecting)
  {
    this->Balloon->SetImageProperty(this->SelectingProperty);
    selectedProperty = this->SelectingProperty;
  }
  else // if ( highlight == svtkButtonRepresentation::HighlightNormal )
  {
    this->Balloon->SetImageProperty(this->Property);
    selectedProperty = this->Property;
  }

  if (selectedProperty != initialProperty)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::CreateDefaultProperties()
{
  this->Property = svtkProperty2D::New();
  this->Property->SetColor(0.9, 0.9, 0.9);

  this->HoveringProperty = svtkProperty2D::New();
  this->HoveringProperty->SetColor(1, 1, 1);

  this->SelectingProperty = svtkProperty2D::New();
  this->SelectingProperty->SetColor(0.5, 0.5, 0.5);
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::BuildRepresentation()
{
  // The net effect is to resize the handle
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime) ||
    (this->Renderer && this->Renderer->GetSVTKWindow() &&
      this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime))
  {
    this->Balloon->SetRenderer(this->Renderer);

    // Setup the texture
    svtkTextureArrayIterator iter = this->TextureArray->find(this->State);
    if (iter != this->TextureArray->end())
    {
      this->Balloon->SetBalloonImage((*iter).second);
    }
    else
    {
      this->Balloon->SetBalloonImage(nullptr);
    }

    // Update the position if anchored in world coordinates
    if (this->Anchor)
    {
      double* p = this->Anchor->GetComputedDoubleDisplayValue(this->Renderer);
      this->Balloon->StartWidgetInteraction(p);
      this->Balloon->Modified();
    }

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::ShallowCopy(svtkProp* prop)
{
  svtkTexturedButtonRepresentation2D* rep = svtkTexturedButtonRepresentation2D::SafeDownCast(prop);
  if (rep)
  {
    this->Property->DeepCopy(rep->Property);
    this->HoveringProperty->DeepCopy(rep->HoveringProperty);
    this->SelectingProperty->DeepCopy(rep->SelectingProperty);

    svtkTextureArrayIterator iter;
    for (iter = rep->TextureArray->begin(); iter != rep->TextureArray->end(); ++iter)
    {
      (*this->TextureArray)[(*iter).first] = (*iter).second;
    }
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Balloon->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkTexturedButtonRepresentation2D::RenderOverlay(svtkViewport* viewport)
{
  this->BuildRepresentation();

  return this->Balloon->RenderOverlay(viewport);
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkTexturedButtonRepresentation2D::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  return this->Balloon->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------
double* svtkTexturedButtonRepresentation2D::GetBounds()
{
  return nullptr;
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::GetActors(svtkPropCollection* pc)
{
  this->Balloon->GetActors(pc);
}

//----------------------------------------------------------------------
void svtkTexturedButtonRepresentation2D::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if (this->HoveringProperty)
  {
    os << indent << "Hovering Property: " << this->HoveringProperty << "\n";
  }
  else
  {
    os << indent << "Hovering Property: (none)\n";
  }

  if (this->SelectingProperty)
  {
    os << indent << "Selecting Property: " << this->SelectingProperty << "\n";
  }
  else
  {
    os << indent << "Selecting Property: (none)\n";
  }
}
