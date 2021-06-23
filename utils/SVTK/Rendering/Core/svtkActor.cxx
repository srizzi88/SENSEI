/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPropCollection.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkScalarsToColors.h"
#include "svtkTexture.h"
#include "svtkTransform.h"

#include <cmath>

svtkCxxSetObjectMacro(svtkActor, Texture, svtkTexture);
svtkCxxSetObjectMacro(svtkActor, Mapper, svtkMapper);
svtkCxxSetObjectMacro(svtkActor, BackfaceProperty, svtkProperty);
svtkCxxSetObjectMacro(svtkActor, Property, svtkProperty);

svtkObjectFactoryNewMacro(svtkActor);

// Creates an actor with the following defaults: origin(0,0,0)
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.
svtkActor::svtkActor()
{
  this->Mapper = nullptr;
  this->Property = nullptr;
  this->BackfaceProperty = nullptr;
  this->Texture = nullptr;

  this->ForceOpaque = false;
  this->ForceTranslucent = false;
  this->InTranslucentPass = false;

  // The mapper bounds are cache to know when the bounds must be recomputed
  // from the mapper bounds.
  svtkMath::UninitializeBounds(this->MapperBounds);
}

//----------------------------------------------------------------------------
svtkActor::~svtkActor()
{
  if (this->Property != nullptr)
  {
    this->Property->UnRegister(this);
    this->Property = nullptr;
  }

  if (this->BackfaceProperty != nullptr)
  {
    this->BackfaceProperty->UnRegister(this);
    this->BackfaceProperty = nullptr;
  }

  if (this->Mapper)
  {
    this->Mapper->UnRegister(this);
    this->Mapper = nullptr;
  }
  this->SetTexture(nullptr);
}

//----------------------------------------------------------------------------
// Shallow copy of an actor.
void svtkActor::ShallowCopy(svtkProp* prop)
{
  svtkActor* a = svtkActor::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetMapper(a->GetMapper());
    this->SetProperty(a->GetProperty());
    this->SetBackfaceProperty(a->GetBackfaceProperty());
    this->SetTexture(a->GetTexture());
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void svtkActor::GetActors(svtkPropCollection* ac)
{
  ac->AddItem(this);
}

svtkTypeBool svtkActor::HasOpaqueGeometry()
{
  if (this->ForceOpaque)
  {
    return 1;
  }
  if (this->ForceTranslucent)
  {
    return 0;
  }

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }
  bool is_opaque = (this->Property->GetOpacity() >= 1.0);

  // are we using an opaque texture, if any?
  is_opaque = is_opaque && (this->Texture == nullptr || this->Texture->IsTranslucent() == 0);

  // are we using an opaque scalar array, if any?
  is_opaque = is_opaque && (this->Mapper == nullptr || this->Mapper->HasOpaqueGeometry());

  return is_opaque ? 1 : 0;
}

svtkTypeBool svtkActor::HasTranslucentPolygonalGeometry()
{
  if (this->ForceOpaque)
  {
    return 0;
  }
  if (this->ForceTranslucent)
  {
    return 1;
  }

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  if (this->Property->GetOpacity() < 1.0)
  {
    return 1;
  }

  if (this->Texture != nullptr && this->Texture->IsTranslucent())
  {
    return 1;
  }

  if (this->Mapper != nullptr && this->Mapper->HasTranslucentPolygonalGeometry())
  {
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// should be called from the render methods only
int svtkActor::GetIsOpaque()
{
  return this->HasOpaqueGeometry();
}

//----------------------------------------------------------------------------
// This causes the actor to be rendered. It in turn will render the actor's
// property, texture map and then mapper. If a property hasn't been
// assigned, then the actor will create one automatically. Note that a
// side effect of this method is that the visualization network is updated.
int svtkActor::RenderOpaqueGeometry(svtkViewport* vp)
{
  int renderedSomething = 0;
  svtkRenderer* ren = static_cast<svtkRenderer*>(vp);

  if (!this->Mapper)
  {
    return 0;
  }

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  // Should we render during the opaque pass?
  if (this->HasOpaqueGeometry() || (ren->GetSelector() && this->Property->GetOpacity() > 0.0))
  {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
    {
      this->BackfaceProperty->BackfaceRender(this, ren);
    }

    // render the texture
    if (this->Texture)
    {
      this->Texture->Render(ren);
      if (this->Texture->GetTransform())
      {
        svtkInformation* info = this->GetPropertyKeys();
        if (!info)
        {
          info = svtkInformation::New();
          this->SetPropertyKeys(info);
          info->Delete();
        }
        info->Set(svtkProp::GeneralTextureTransform(),
          &(this->Texture->GetTransform()->GetMatrix()->Element[0][0]), 16);
      }
    }
    this->Render(ren, this->Mapper);
    this->Property->PostRender(this, ren);
    if (this->Texture)
    {
      this->Texture->PostRender(ren);
      if (this->Texture->GetTransform())
      {
        svtkInformation* info = this->GetPropertyKeys();
        info->Remove(svtkProp::GeneralTextureTransform());
      }
    }
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
    renderedSomething = 1;
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int svtkActor::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  int renderedSomething = 0;
  svtkRenderer* ren = static_cast<svtkRenderer*>(vp);

  if (!this->Mapper)
  {
    return 0;
  }

  this->InTranslucentPass = true;

  // make sure we have a property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }

  // Should we render during the translucent pass?
  if (this->HasTranslucentPolygonalGeometry() && !ren->GetSelector())
  {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
    {
      this->BackfaceProperty->BackfaceRender(this, ren);
    }

    // render the texture
    if (this->Texture)
    {
      this->Texture->Render(ren);
      if (this->Texture->GetTransform())
      {
        svtkInformation* info = this->GetPropertyKeys();
        if (!info)
        {
          info = svtkInformation::New();
          this->SetPropertyKeys(info);
          info->Delete();
        }
        info->Set(svtkProp::GeneralTextureTransform(),
          &(this->Texture->GetTransform()->GetMatrix()->Element[0][0]), 16);
      }
    }
    this->Render(ren, this->Mapper);
    this->Property->PostRender(this, ren);
    if (this->Texture)
    {
      this->Texture->PostRender(ren);
      if (this->Texture->GetTransform())
      {
        svtkInformation* info = this->GetPropertyKeys();
        info->Remove(svtkProp::GeneralTextureTransform());
      }
    }
    this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();

    renderedSomething = 1;
  }

  this->InTranslucentPass = false;
  return renderedSomething;
}

//----------------------------------------------------------------------------
void svtkActor::ReleaseGraphicsResources(svtkWindow* win)
{
  svtkRenderWindow* renWin = static_cast<svtkRenderWindow*>(win);

  // pass this information onto the mapper
  if (this->Mapper)
  {
    this->Mapper->ReleaseGraphicsResources(renWin);
  }

  // pass this information onto the texture
  if (this->Texture)
  {
    this->Texture->ReleaseGraphicsResources(renWin);
  }

  // pass this information to the properties
  if (this->Property)
  {
    this->Property->ReleaseGraphicsResources(renWin);
  }
  if (this->BackfaceProperty)
  {
    this->BackfaceProperty->ReleaseGraphicsResources(renWin);
  }
}

//----------------------------------------------------------------------------
svtkProperty* svtkActor::MakeProperty()
{
  return svtkProperty::New();
}

//----------------------------------------------------------------------------
svtkProperty* svtkActor::GetProperty()
{
  if (this->Property == nullptr)
  {
    svtkProperty* p = this->MakeProperty();
    this->SetProperty(p);
    p->Delete();
  }
  return this->Property;
}

//----------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkActor::GetBounds()
{
  int i, n;
  double bbox[24], *fptr;

  svtkDebugMacro(<< "Getting Bounds");

  // get the bounds of the Mapper if we have one
  if (!this->Mapper)
  {
    return this->Bounds;
  }

  const double* bounds = this->Mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
  {
    return nullptr;
  }

  // Check for the special case when the actor is empty.
  if (!svtkMath::AreBoundsInitialized(bounds))
  {
    memcpy(this->MapperBounds, bounds, 6 * sizeof(double));
    svtkMath::UninitializeBounds(this->Bounds);
    this->BoundsMTime.Modified();
    return this->Bounds;
  }

  // Check if we have cached values for these bounds - we cache the
  // values returned by this->Mapper->GetBounds() and we store the time
  // of caching. If the values returned this time are different, or
  // the modified time of this class is newer than the cached time,
  // then we need to rebuild.
  if ((memcmp(this->MapperBounds, bounds, 6 * sizeof(double)) != 0) ||
    (this->GetMTime() > this->BoundsMTime))
  {
    svtkDebugMacro(<< "Recomputing bounds...");

    memcpy(this->MapperBounds, bounds, 6 * sizeof(double));

    // fill out vertices of a bounding box
    bbox[0] = bounds[1];
    bbox[1] = bounds[3];
    bbox[2] = bounds[5];
    bbox[3] = bounds[1];
    bbox[4] = bounds[2];
    bbox[5] = bounds[5];
    bbox[6] = bounds[0];
    bbox[7] = bounds[2];
    bbox[8] = bounds[5];
    bbox[9] = bounds[0];
    bbox[10] = bounds[3];
    bbox[11] = bounds[5];
    bbox[12] = bounds[1];
    bbox[13] = bounds[3];
    bbox[14] = bounds[4];
    bbox[15] = bounds[1];
    bbox[16] = bounds[2];
    bbox[17] = bounds[4];
    bbox[18] = bounds[0];
    bbox[19] = bounds[2];
    bbox[20] = bounds[4];
    bbox[21] = bounds[0];
    bbox[22] = bounds[3];
    bbox[23] = bounds[4];

    // make sure matrix (transform) is up-to-date
    this->ComputeMatrix();

    // and transform into actors coordinates
    fptr = bbox;
    for (n = 0; n < 8; n++)
    {
      double homogeneousPt[4] = { fptr[0], fptr[1], fptr[2], 1.0 };
      this->Matrix->MultiplyPoint(homogeneousPt, homogeneousPt);
      fptr[0] = homogeneousPt[0] / homogeneousPt[3];
      fptr[1] = homogeneousPt[1] / homogeneousPt[3];
      fptr[2] = homogeneousPt[2] / homogeneousPt[3];
      fptr += 3;
    }

    // now calc the new bounds
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = SVTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -SVTK_DOUBLE_MAX;
    for (i = 0; i < 8; i++)
    {
      for (n = 0; n < 3; n++)
      {
        if (bbox[i * 3 + n] < this->Bounds[n * 2])
        {
          this->Bounds[n * 2] = bbox[i * 3 + n];
        }
        if (bbox[i * 3 + n] > this->Bounds[n * 2 + 1])
        {
          this->Bounds[n * 2 + 1] = bbox[i * 3 + n];
        }
      }
    }
    this->BoundsMTime.Modified();
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkActor::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Property != nullptr)
  {
    time = this->Property->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if (this->BackfaceProperty != nullptr)
  {
    time = this->BackfaceProperty->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if (this->Texture != nullptr)
  {
    time = this->Texture->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkActor::GetRedrawMTime()
{
  svtkMTimeType mTime = this->GetMTime();
  svtkMTimeType time;

  svtkMapper* myMapper = this->GetMapper();
  if (myMapper != nullptr)
  {
    time = myMapper->GetMTime();
    mTime = (time > mTime ? time : mTime);
    if (myMapper->GetNumberOfInputPorts() > 0 && myMapper->GetInput() != nullptr)
    {
      myMapper->GetInputAlgorithm()->Update();
      time = myMapper->GetInput()->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Mapper)
  {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Mapper: (none)\n";
  }

  if (this->Property)
  {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if (this->BackfaceProperty)
  {
    os << indent << "BackfaceProperty:\n";
    this->BackfaceProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "BackfaceProperty: (none)\n";
  }

  if (this->Texture)
  {
    os << indent << "Texture: " << this->Texture << "\n";
  }
  else
  {
    os << indent << "Texture: (none)\n";
  }

  os << indent << "ForceOpaque: " << (this->ForceOpaque ? "true" : "false") << "\n";
  os << indent << "ForceTranslucent: " << (this->ForceTranslucent ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
bool svtkActor::GetSupportsSelection()
{
  if (this->Mapper)
  {
    return this->Mapper->GetSupportsSelection();
  }

  return false;
}

void svtkActor::ProcessSelectorPixelBuffers(
  svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets)
{
  if (this->Mapper)
  {
    this->Mapper->ProcessSelectorPixelBuffers(sel, pixeloffsets, this);
  }
}
