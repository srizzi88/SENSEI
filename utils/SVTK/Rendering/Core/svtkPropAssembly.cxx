/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPropAssembly.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkAssemblyPaths.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkPropCollection.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkPropAssembly);

// Construct object with no children.
svtkPropAssembly::svtkPropAssembly()
{
  this->Parts = svtkPropCollection::New();
  svtkMath::UninitializeBounds(this->Bounds);
}

svtkPropAssembly::~svtkPropAssembly()
{
  svtkCollectionSimpleIterator pit;
  svtkProp* part;
  for (this->Parts->InitTraversal(pit); (part = this->Parts->GetNextProp(pit));)
  {
    part->RemoveConsumer(this);
  }

  this->Parts->Delete();
  this->Parts = nullptr;
}

// Add a part to the list of Parts.
void svtkPropAssembly::AddPart(svtkProp* prop)
{
  if (!this->Parts->IsItemPresent(prop))
  {
    this->Parts->AddItem(prop);
    prop->AddConsumer(this);
    this->Modified();
  }
}

// Remove a part from the list of parts,
void svtkPropAssembly::RemovePart(svtkProp* prop)
{
  if (this->Parts->IsItemPresent(prop))
  {
    prop->RemoveConsumer(this);
    this->Parts->RemoveItem(prop);
    this->Modified();
  }
}

// Get the list of parts for this prop assembly.
svtkPropCollection* svtkPropAssembly::GetParts()
{
  return this->Parts;
}

// Render this assembly and all of its Parts. The rendering process is recursive.
int svtkPropAssembly::RenderTranslucentPolygonalGeometry(svtkViewport* ren)
{
  svtkProp* prop;
  svtkAssemblyPath* path;
  double fraction;
  int renderedSomething = 0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  double numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
  fraction =
    numberOfItems >= 1.0 ? this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

  // render the Paths
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    prop = path->GetLastNode()->GetViewProp();
    if (prop->GetVisibility())
    {
      prop->SetPropertyKeys(this->GetPropertyKeys());
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderTranslucentPolygonalGeometry(ren);
      prop->PokeMatrix(nullptr);
    }
  }

  return renderedSomething;
}

// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkPropAssembly::HasTranslucentPolygonalGeometry()
{
  svtkProp* prop;
  svtkAssemblyPath* path;
  int result = 0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  // render the Paths
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); !result && (path = this->Paths->GetNextPath(sit));)
  {
    prop = path->GetLastNode()->GetViewProp();
    if (prop->GetVisibility())
    {
      prop->SetPropertyKeys(this->GetPropertyKeys());
      result = prop->HasTranslucentPolygonalGeometry();
    }
  }
  return result;
}

// Render this assembly and all of its Parts. The rendering process is recursive.
int svtkPropAssembly::RenderVolumetricGeometry(svtkViewport* ren)
{
  svtkProp* prop;
  svtkAssemblyPath* path;
  double fraction;
  int renderedSomething = 0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  double numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
  fraction =
    numberOfItems >= 1.0 ? this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

  // render the Paths
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    prop = path->GetLastNode()->GetViewProp();
    if (prop->GetVisibility())
    {
      prop->SetPropertyKeys(this->GetPropertyKeys());
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderVolumetricGeometry(ren);
      prop->PokeMatrix(nullptr);
    }
  }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int svtkPropAssembly::RenderOpaqueGeometry(svtkViewport* ren)
{
  svtkProp* prop;
  svtkAssemblyPath* path;
  double fraction;
  int renderedSomething = 0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  double numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
  fraction =
    numberOfItems >= 1.0 ? this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

  // render the Paths
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    prop = path->GetLastNode()->GetViewProp();
    if (prop->GetVisibility())
    {
      prop->SetPropertyKeys(this->GetPropertyKeys());
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderOpaqueGeometry(ren);
      prop->PokeMatrix(nullptr);
    }
  }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int svtkPropAssembly::RenderOverlay(svtkViewport* ren)
{
  svtkProp* prop;
  svtkAssemblyPath* path;
  double fraction;
  int renderedSomething = 0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  double numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
  fraction =
    numberOfItems >= 1.0 ? this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    prop = path->GetLastNode()->GetViewProp();
    if (prop->GetVisibility())
    {
      prop->SetPropertyKeys(this->GetPropertyKeys());
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderOverlay(ren);
      prop->PokeMatrix(nullptr);
    }
  }

  return renderedSomething;
}

void svtkPropAssembly::ReleaseGraphicsResources(svtkWindow* renWin)
{
  svtkProp* part;

  svtkProp::ReleaseGraphicsResources(renWin);

  // broadcast the message down the Parts
  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (part = this->Parts->GetNextProp(pit));)
  {
    part->ReleaseGraphicsResources(renWin);
  }
}

// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkPropAssembly::GetBounds()
{
  svtkProp* part;
  int i, n;
  double bbox[24];
  int partVisible = 0;

  // carefully compute the bounds
  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (part = this->Parts->GetNextProp(pit));)
  {
    if (part->GetVisibility() && part->GetUseBounds())
    {
      const double* bounds = part->GetBounds();

      if (bounds != nullptr)
      {
        //  For the purposes of GetBounds, an object is visible only if
        //  its visibility is on and it has visible parts.
        if (!partVisible)
        {
          // initialize the bounds
          this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = SVTK_DOUBLE_MAX;
          this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -SVTK_DOUBLE_MAX;
          partVisible = 1;
        }

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
        } // for each point of box
      }   // if bounds
    }     // for each part
  }       // for each part

  if (!partVisible)
  {
    return nullptr;
  }
  else
  {
    return this->Bounds;
  }
}

svtkMTimeType svtkPropAssembly::GetMTime()
{
  svtkMTimeType mTime = this->svtkProp::GetMTime();
  svtkMTimeType time;
  svtkProp* part;

  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (part = this->Parts->GetNextProp(pit));)
  {
    time = part->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

// Shallow copy another svtkPropAssembly.
void svtkPropAssembly::ShallowCopy(svtkProp* prop)
{
  svtkPropAssembly* propAssembly = svtkPropAssembly::SafeDownCast(prop);
  if (propAssembly != nullptr && propAssembly != this)
  {
    svtkCollectionSimpleIterator pit;
    svtkProp* part;
    for (this->Parts->InitTraversal(pit); (part = this->Parts->GetNextProp(pit));)
    {
      part->RemoveConsumer(this);
    }
    this->Parts->RemoveAllItems();
    for (propAssembly->Parts->InitTraversal(pit); (part = propAssembly->Parts->GetNextProp(pit));)
    {
      this->AddPart(part);
    }
  }

  this->svtkProp::ShallowCopy(prop);
}

void svtkPropAssembly::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

svtkAssemblyPath* svtkPropAssembly::GetNextPath()
{
  if (this->Paths)
  {
    return this->Paths->GetNextItem();
  }
  return nullptr;
}

int svtkPropAssembly::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}

// Build the assembly paths if necessary.
void svtkPropAssembly::UpdatePaths()
{
  if (this->GetMTime() > this->PathTime)
  {
    if (this->Paths != nullptr)
    {
      this->Paths->Delete();
      this->Paths = nullptr;
    }

    // Create the list to hold all the paths
    this->Paths = svtkAssemblyPaths::New();
    svtkAssemblyPath* path = svtkAssemblyPath::New();

    // add ourselves to the path to start things off
    path->AddNode(this, nullptr);

    svtkProp* prop;
    // Add nodes as we proceed down the hierarchy
    svtkCollectionSimpleIterator pit;
    for (this->Parts->InitTraversal(pit); (prop = this->Parts->GetNextProp(pit));)
    {
      // add a matrix, if any
      path->AddNode(prop, prop->GetMatrix());

      // dive into the hierarchy
      prop->BuildPaths(this->Paths, path);

      // when returned, pop the last node off of the
      // current path
      path->DeleteLastNode();
    }

    path->Delete();
    this->PathTime.Modified();
  }
}

void svtkPropAssembly::BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path)
{
  svtkProp* prop;

  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (prop = this->Parts->GetNextProp(pit));)
  {
    path->AddNode(prop, nullptr);

    // dive into the hierarchy
    prop->BuildPaths(paths, path);

    // when returned, pop the last node off of the
    // current path
    path->DeleteLastNode();
  }
}

void svtkPropAssembly::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems() << " parts in this assembly\n";
}
