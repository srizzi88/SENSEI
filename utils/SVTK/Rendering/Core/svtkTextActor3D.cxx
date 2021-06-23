/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTextActor3D.h"

#include "svtkCamera.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkTransform.h"
#include "svtkWindow.h"

svtkObjectFactoryNewMacro(svtkTextActor3D);

svtkCxxSetObjectMacro(svtkTextActor3D, TextProperty, svtkTextProperty);

// ----------------------------------------------------------------------------
svtkTextActor3D::svtkTextActor3D()
{
  this->Input = nullptr;
  this->LastInputString = "";
  this->ImageActor = svtkImageActor::New();
  this->ImageData = nullptr;
  this->TextProperty = nullptr;

  this->BuildTime.Modified();

  this->SetTextProperty(svtkTextProperty::New());
  this->TextProperty->Delete();

  this->ImageActor->InterpolateOn();
}

// --------------------------------------------------------------------------
svtkTextActor3D::~svtkTextActor3D()
{
  this->SetTextProperty(nullptr);
  this->SetInput(nullptr);

  this->ImageActor->Delete();
  this->ImageActor = nullptr;

  if (this->ImageData)
  {
    this->ImageData->Delete();
    this->ImageData = nullptr;
  }
}

// --------------------------------------------------------------------------
void svtkTextActor3D::ShallowCopy(svtkProp* prop)
{
  svtkTextActor3D* a = svtkTextActor3D::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetInput(a->GetInput());
    this->SetTextProperty(a->GetTextProperty());
  }

  this->Superclass::ShallowCopy(prop);
}

// --------------------------------------------------------------------------
double* svtkTextActor3D::GetBounds()
{
  // the culler could be asking our bounds, in which case it's possible
  // that we haven't rendered yet, so we have to make sure our bounds
  // are up to date so that we don't get culled.
  this->UpdateImageActor();
  const double* bounds = this->ImageActor->GetBounds();
  this->Bounds[0] = bounds[0];
  this->Bounds[1] = bounds[1];
  this->Bounds[2] = bounds[2];
  this->Bounds[3] = bounds[3];
  this->Bounds[4] = bounds[4];
  this->Bounds[5] = bounds[5];
  return this->Bounds;
}

// --------------------------------------------------------------------------
int svtkTextActor3D::GetBoundingBox(int bbox[4])
{
  if (!this->TextProperty)
  {
    svtkErrorMacro(<< "Need valid svtkTextProperty.");
    return 0;
  }

  if (!bbox)
  {
    svtkErrorMacro(<< "Need 4-element int array for bounding box.");
    return 0;
  }

  svtkTextRenderer* tRend = svtkTextRenderer::GetInstance();
  if (!tRend)
  {
    svtkErrorMacro(<< "Failed getting the TextRenderer instance.");
    return 0;
  }

  if (!tRend->GetBoundingBox(
        this->TextProperty, this->Input, bbox, svtkTextActor3D::GetRenderedDPI()))
  {
    svtkErrorMacro(<< "No text in input.");
    return 0;
  }

  return 1;
}

// --------------------------------------------------------------------------
void svtkTextActor3D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->ImageActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

// --------------------------------------------------------------------------
void svtkTextActor3D::SetForceOpaque(bool opaque)
{
  this->ImageActor->SetForceOpaque(opaque);
}

// --------------------------------------------------------------------------
bool svtkTextActor3D::GetForceOpaque()
{
  return this->ImageActor->GetForceOpaque();
}

// --------------------------------------------------------------------------
void svtkTextActor3D::ForceOpaqueOn()
{
  this->ImageActor->ForceOpaqueOn();
}

// --------------------------------------------------------------------------
void svtkTextActor3D::ForceOpaqueOff()
{
  this->ImageActor->ForceOpaqueOff();
}

// --------------------------------------------------------------------------
void svtkTextActor3D::SetForceTranslucent(bool trans)
{
  this->ImageActor->SetForceTranslucent(trans);
}

// --------------------------------------------------------------------------
bool svtkTextActor3D::GetForceTranslucent()
{
  return this->ImageActor->GetForceTranslucent();
}

// --------------------------------------------------------------------------
void svtkTextActor3D::ForceTranslucentOn()
{
  this->ImageActor->ForceTranslucentOn();
}

// --------------------------------------------------------------------------
void svtkTextActor3D::ForceTranslucentOff()
{
  this->ImageActor->ForceTranslucentOff();
}

// --------------------------------------------------------------------------
int svtkTextActor3D::RenderOverlay(svtkViewport* viewport)
{
  int rendered_something = 0;

  if (this->UpdateImageActor() && this->ImageData && this->ImageData->GetNumberOfPoints() > 0)
  {
    rendered_something += this->ImageActor->RenderOverlay(viewport);
  }

  return rendered_something;
}

// ----------------------------------------------------------------------------
int svtkTextActor3D::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  int rendered_something = 0;

  if (this->UpdateImageActor() && this->ImageData && this->ImageData->GetNumberOfPoints() > 0)
  {
    rendered_something += this->ImageActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  return rendered_something;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkTextActor3D::HasTranslucentPolygonalGeometry()
{
  this->UpdateImageActor();
  return this->ImageActor->HasTranslucentPolygonalGeometry();
}

// --------------------------------------------------------------------------
int svtkTextActor3D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  int rendered_something = 0;

  if (svtkRenderer* renderer = svtkRenderer::SafeDownCast(viewport))
  {
    if (svtkRenderWindow* renderWindow = renderer->GetRenderWindow())
    {
      // Is the viewport's RenderWindow capturing GL2PS-special props?
      if (renderWindow->GetCapturingGL2PSSpecialProps())
      {
        renderer->CaptureGL2PSSpecialProp(this);
      }
    }
  }

  if (this->UpdateImageActor() && this->ImageData && this->ImageData->GetNumberOfPoints() > 0)
  {
    rendered_something += this->ImageActor->RenderOpaqueGeometry(viewport);
  }

  return rendered_something;
}

// --------------------------------------------------------------------------
int svtkTextActor3D::UpdateImageActor()
{
  // Need text prop
  if (!this->TextProperty)
  {
    svtkErrorMacro(<< "Need a text property to render text actor");
    this->ImageActor->SetInputData(nullptr);
    return 0;
  }

  // No input, the assign the image actor a zilch input
  if (!this->Input || !*this->Input)
  {
    this->ImageActor->SetInputData(nullptr);
    return 1;
  }

  // copy information to the delegate
  svtkInformation* info = this->GetPropertyKeys();
  this->ImageActor->SetPropertyKeys(info);

  // Do we need to (re-)render the text ?
  // Yes if:
  //  - instance has been modified since last build
  //  - text prop has been modified since last build
  //  - ImageData ivar has not been allocated yet
  if (this->GetMTime() > this->BuildTime || this->TextProperty->GetMTime() > this->BuildTime ||
    !this->ImageData)
  {
    // we have to give svtkFTU::RenderString something to work with
    if (!this->ImageData)
    {
      this->ImageData = svtkImageData::New();
      this->ImageData->SetSpacing(1.0, 1.0, 1.0);
    }

    svtkTextRenderer* tRend = svtkTextRenderer::GetInstance();
    if (!tRend)
    {
      svtkErrorMacro(<< "Failed getting the TextRenderer instance.");
      this->ImageActor->SetInputData(nullptr);
      return 0;
    }

    if (this->TextProperty->GetMTime() > this->BuildTime || this->LastInputString != this->Input)
    {
      if (!tRend->RenderString(this->TextProperty, this->Input, this->ImageData, nullptr,
            svtkTextActor3D::GetRenderedDPI()))
      {
        svtkErrorMacro(<< "Failed rendering text to buffer");
        this->ImageActor->SetInputData(nullptr);
        return 0;
      }

      // Associate the image data (should be up to date now) to the image actor
      this->ImageActor->SetInputData(this->ImageData);

      // Only render the visible portions of the texture.
      int bbox[6] = { 0, 0, 0, 0, 0, 0 };
      this->GetBoundingBox(bbox);
      this->ImageActor->SetDisplayExtent(bbox);
      this->LastInputString = this->Input;
    }

    this->BuildTime.Modified();
  } // if (this->GetMTime() ...

  // Position the actor
  svtkMatrix4x4* matrix = this->ImageActor->GetUserMatrix();
  if (!matrix)
  {
    matrix = svtkMatrix4x4::New();
    this->ImageActor->SetUserMatrix(matrix);
    matrix->Delete();
  }
  this->GetMatrix(matrix);

  return 1;
}

// --------------------------------------------------------------------------
void svtkTextActor3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << (this->Input ? this->Input : "(none)") << "\n";

  if (this->TextProperty)
  {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Text Property: (none)\n";
  }
}
