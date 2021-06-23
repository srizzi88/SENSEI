/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStack.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageStack.h"
#include "svtkAssemblyPath.h"
#include "svtkAssemblyPaths.h"
#include "svtkImageMapper3D.h"
#include "svtkImageProperty.h"
#include "svtkImageSliceCollection.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkImageStack);

//----------------------------------------------------------------------------
svtkImageStack::svtkImageStack()
{
  this->Images = svtkImageSliceCollection::New();
  this->ImageMatrices = nullptr;
  this->ActiveLayer = 0;
}

//----------------------------------------------------------------------------
svtkImageStack::~svtkImageStack()
{
  if (this->Images)
  {
    svtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    svtkImageSlice* image = nullptr;
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      image->RemoveConsumer(this);
    }

    this->Images->Delete();
  }

  if (this->ImageMatrices)
  {
    this->ImageMatrices->Delete();
  }
}

//----------------------------------------------------------------------------
svtkImageSlice* svtkImageStack::GetActiveImage()
{
  svtkImageSlice* activeImage = nullptr;

  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    svtkImageProperty* p = image->GetProperty();
    if (p->GetLayerNumber() == this->ActiveLayer)
    {
      activeImage = image;
    }
  }

  return activeImage;
}

//----------------------------------------------------------------------------
void svtkImageStack::AddImage(svtkImageSlice* prop)
{
  if (!this->Images->IsItemPresent(prop) && !svtkImageStack::SafeDownCast(prop))
  {
    this->Images->AddItem(prop);
    prop->AddConsumer(this);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageStack::RemoveImage(svtkImageSlice* prop)
{
  if (this->Images->IsItemPresent(prop))
  {
    prop->RemoveConsumer(this);
    this->Images->RemoveItem(prop);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkImageStack::HasImage(svtkImageSlice* prop)
{
  return this->Images->IsItemPresent(prop);
}

//----------------------------------------------------------------------------
void svtkImageStack::GetImages(svtkPropCollection* vc)
{
  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    image->GetImages(vc);
  }
}

//----------------------------------------------------------------------------
void svtkImageStack::ShallowCopy(svtkProp* prop)
{
  svtkImageStack* v = svtkImageStack::SafeDownCast(prop);

  if (v != nullptr)
  {
    this->Images->RemoveAllItems();
    svtkCollectionSimpleIterator pit;
    v->Images->InitTraversal(pit);
    svtkImageSlice* image = nullptr;
    while ((image = v->Images->GetNextImage(pit)) != nullptr)
    {
      this->Images->AddItem(image);
    }
    this->SetActiveLayer(v->GetActiveLayer());
  }

  // Now do prop superclass (NOT svtkImageSlice)
  this->svtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void svtkImageStack::SetProperty(svtkImageProperty*)
{
  // do nothing
}

//----------------------------------------------------------------------------
svtkImageProperty* svtkImageStack::GetProperty()
{
  // Get the property with the active layer number
  svtkImageSlice* image = this->GetActiveImage();
  if (image)
  {
    return image->GetProperty();
  }

  // Return a dummy property, can't return nullptr.
  if (this->Property == nullptr)
  {
    this->Property = svtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
  }
  return this->Property;
}

//----------------------------------------------------------------------------
void svtkImageStack::SetMapper(svtkImageMapper3D*)
{
  // do nothing
}

//----------------------------------------------------------------------------
svtkImageMapper3D* svtkImageStack::GetMapper()
{
  // Get the mapper with the active layer number
  svtkImageSlice* image = this->GetActiveImage();
  if (image)
  {
    return image->GetMapper();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
double* svtkImageStack::GetBounds()
{
  this->UpdatePaths();

  double bounds[6];
  bool nobounds = true;

  bounds[0] = SVTK_DOUBLE_MAX;
  bounds[1] = SVTK_DOUBLE_MIN;
  bounds[2] = SVTK_DOUBLE_MAX;
  bounds[3] = SVTK_DOUBLE_MIN;
  bounds[4] = SVTK_DOUBLE_MAX;
  bounds[5] = SVTK_DOUBLE_MIN;

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    const double* b = image->GetBounds();
    if (b)
    {
      nobounds = false;
      bounds[0] = (bounds[0] < b[0] ? bounds[0] : b[0]);
      bounds[1] = (bounds[1] > b[1] ? bounds[1] : b[1]);
      bounds[2] = (bounds[2] < b[2] ? bounds[2] : b[2]);
      bounds[3] = (bounds[3] > b[3] ? bounds[3] : b[3]);
      bounds[4] = (bounds[4] < b[4] ? bounds[4] : b[4]);
      bounds[5] = (bounds[5] > b[5] ? bounds[5] : b[5]);
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  if (nobounds)
  {
    return nullptr;
  }

  this->Bounds[0] = bounds[0];
  this->Bounds[1] = bounds[1];
  this->Bounds[2] = bounds[2];
  this->Bounds[3] = bounds[3];
  this->Bounds[4] = bounds[4];
  this->Bounds[5] = bounds[5];

  return this->Bounds;
}

//----------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkImageStack::HasTranslucentPolygonalGeometry()
{
  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    if (image->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
// Assembly-like behavior
void svtkImageStack::PokeMatrices(svtkMatrix4x4* matrix)
{
  if (this->ImageMatrices == nullptr)
  {
    this->ImageMatrices = svtkCollection::New();
  }

  if (matrix)
  {
    svtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    svtkImageSlice* image = nullptr;
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      svtkMatrix4x4* propMatrix = svtkMatrix4x4::New();
      propMatrix->Multiply4x4(image->GetMatrix(), matrix, propMatrix);
      image->PokeMatrix(propMatrix);
      this->ImageMatrices->AddItem(propMatrix);
      propMatrix->Delete();
    }
  }
  else
  {
    svtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    svtkImageSlice* image = nullptr;
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      image->PokeMatrix(nullptr);
    }
    this->ImageMatrices->RemoveAllItems();
  }
}

//----------------------------------------------------------------------------
int svtkImageStack::RenderOpaqueGeometry(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkImageStack::RenderOpaqueGeometry");

  // Opaque render is always called first, so sort here
  this->Images->Sort();
  this->UpdatePaths();

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  int rendered = 0;
  svtkImageSlice* image = nullptr;
  svtkCollectionSimpleIterator pit;
  svtkIdType n = 0;
  this->Images->InitTraversal(pit);
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    n += (image->GetVisibility() != 0);
  }
  double renderTime = this->AllocatedRenderTime / (n + (n == 0));

  if (n == 1)
  {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      if (image->GetVisibility())
      {
        image->SetAllocatedRenderTime(renderTime, viewport);
        rendered = image->RenderOpaqueGeometry(viewport);
      }
    }
  }
  else
  {
    for (int pass = 0; pass < 3; pass++)
    {
      this->Images->InitTraversal(pit);
      while ((image = this->Images->GetNextImage(pit)) != nullptr)
      {
        if (image->GetVisibility())
        {
          image->SetAllocatedRenderTime(renderTime, viewport);
          image->SetStackedImagePass(pass);
          rendered |= image->RenderOpaqueGeometry(viewport);
          image->SetStackedImagePass(-1);
        }
      }
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  return rendered;
}

//----------------------------------------------------------------------------
int svtkImageStack::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkImageStack::RenderTranslucentPolygonalGeometry");

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  int rendered = 0;
  svtkImageSlice* image = nullptr;
  svtkCollectionSimpleIterator pit;
  svtkIdType n = 0;
  this->Images->InitTraversal(pit);
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    n += (image->GetVisibility() != 0);
  }
  double renderTime = this->AllocatedRenderTime / (n + (n == 0));

  if (n == 1)
  {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      if (image->GetVisibility())
      {
        image->SetAllocatedRenderTime(renderTime, viewport);
        rendered = image->RenderTranslucentPolygonalGeometry(viewport);
      }
    }
  }
  else
  {
    for (int pass = 1; pass < 3; pass++)
    {
      this->Images->InitTraversal(pit);
      while ((image = this->Images->GetNextImage(pit)) != nullptr)
      {
        if (image->GetVisibility())
        {
          image->SetAllocatedRenderTime(renderTime, viewport);
          image->SetStackedImagePass(pass);
          rendered |= image->RenderTranslucentPolygonalGeometry(viewport);
          image->SetStackedImagePass(-1);
        }
      }
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  return rendered;
}

//----------------------------------------------------------------------------
int svtkImageStack::RenderOverlay(svtkViewport* viewport)
{
  svtkDebugMacro(<< "svtkImageStack::RenderOverlay");

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  int rendered = 0;
  svtkImageSlice* image = nullptr;
  svtkCollectionSimpleIterator pit;
  svtkIdType n = 0;
  this->Images->InitTraversal(pit);
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    n += (image->GetVisibility() != 0);
  }
  double renderTime = this->AllocatedRenderTime / (n + (n == 0));

  if (n == 1)
  {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      if (image->GetVisibility())
      {
        image->SetAllocatedRenderTime(renderTime, viewport);
        rendered = image->RenderOverlay(viewport);
      }
    }
  }
  else
  {
    for (int pass = 1; pass < 3; pass++)
    {
      this->Images->InitTraversal(pit);
      while ((image = this->Images->GetNextImage(pit)) != nullptr)
      {
        if (image->GetVisibility())
        {
          image->SetAllocatedRenderTime(renderTime, viewport);
          image->SetStackedImagePass(pass);
          rendered |= image->RenderOverlay(viewport);
          image->SetStackedImagePass(-1);
        }
      }
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  return rendered;
}

//----------------------------------------------------------------------------
void svtkImageStack::ReleaseGraphicsResources(svtkWindow* win)
{
  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    image->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageStack::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType t;

  // Get the max mtime of all the images
  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    t = image->GetMTime();
    mTime = (t < mTime ? mTime : t);
  }

  return mTime;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageStack::GetRedrawMTime()
{
  // Just call GetMTime on ourselves, not GetRedrawMTime
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType t;

  // Get the max mtime of all the images
  svtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  svtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    t = image->GetRedrawMTime();
    mTime = (t < mTime ? mTime : t);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkImageStack::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

//----------------------------------------------------------------------------
svtkAssemblyPath* svtkImageStack::GetNextPath()
{
  if (this->Paths)
  {
    return this->Paths->GetNextItem();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkImageStack::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void svtkImageStack::UpdatePaths()
{
  if (this->GetMTime() > this->PathTime ||
    (this->Paths && this->Paths->GetMTime() > this->PathTime))
  {
    if (this->Paths)
    {
      this->Paths->Delete();
    }

    // Create the list to hold all the paths
    this->Paths = svtkAssemblyPaths::New();
    svtkAssemblyPath* path = svtkAssemblyPath::New();

    // Add ourselves to the path to start things off
    path->AddNode(this, this->GetMatrix());

    // Add the active image
    svtkImageSlice* image = this->GetActiveImage();

    if (image)
    {
      path->AddNode(image, image->GetMatrix());
      image->BuildPaths(this->Paths, path);
      path->DeleteLastNode();
    }

    path->Delete();
    this->PathTime.Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageStack::BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path)
{
  // the path consists only of the active image
  svtkImageSlice* image = this->GetActiveImage();

  if (image)
  {
    path->AddNode(image, image->GetMatrix());
    image->BuildPaths(paths, path);
    path->DeleteLastNode();
  }
}

//----------------------------------------------------------------------------
void svtkImageStack::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Images: " << this->Images << "\n";
  os << indent << "ActiveLayer: " << this->ActiveLayer << "\n";
  os << indent << "ActiveImage: " << this->GetActiveImage() << "\n";
}
