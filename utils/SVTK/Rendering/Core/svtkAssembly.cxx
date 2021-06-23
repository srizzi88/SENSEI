/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAssembly.h"
#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPaths.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkProp3DCollection.h"
#include "svtkRenderWindow.h"
#include "svtkVolume.h"

svtkStandardNewMacro(svtkAssembly);

// Construct object with no children.
svtkAssembly::svtkAssembly()
{
  this->Parts = svtkProp3DCollection::New();
}

svtkAssembly::~svtkAssembly()
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
void svtkAssembly::AddPart(svtkProp3D* prop)
{
  if (!this->Parts->IsItemPresent(prop))
  {
    this->Parts->AddItem(prop);
    prop->AddConsumer(this);
    this->Modified();
  }
}

// Remove a part from the list of parts,
void svtkAssembly::RemovePart(svtkProp3D* prop)
{
  if (this->Parts->IsItemPresent(prop))
  {
    prop->RemoveConsumer(this);
    this->Parts->RemoveItem(prop);
    this->Modified();
  }
}

// Shallow copy another assembly.
void svtkAssembly::ShallowCopy(svtkProp* prop)
{
  svtkAssembly* p = svtkAssembly::SafeDownCast(prop);
  if (p && p != this)
  {
    svtkCollectionSimpleIterator pit;
    svtkProp3D* part;
    for (this->Parts->InitTraversal(pit); (part = this->Parts->GetNextProp3D(pit));)
    {
      part->RemoveConsumer(this);
    }
    this->Parts->RemoveAllItems();
    for (p->Parts->InitTraversal(pit); (part = p->Parts->GetNextProp3D(pit));)
    {
      this->AddPart(part);
    }
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}

// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int svtkAssembly::RenderTranslucentPolygonalGeometry(svtkViewport* ren)
{
  this->UpdatePaths();

  int renderedSomething = 0;

  // for allocating render time between components
  // simple equal allocation
  double fraction =
    this->AllocatedRenderTime / static_cast<double>(this->Paths->GetNumberOfItems());

  // render the Paths
  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility())
    {
      prop3D->SetPropertyKeys(this->GetPropertyKeys());
      prop3D->SetAllocatedRenderTime(fraction, ren);
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop3D->RenderTranslucentPolygonalGeometry(ren);
      prop3D->PokeMatrix(nullptr);
    }
  }

  return (renderedSomething > 0) ? 1 : 0;
}

// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkAssembly::HasTranslucentPolygonalGeometry()
{
  this->UpdatePaths();

  int result = 0;

  // render the Paths
  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); !result && (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility())
    {
      prop3D->SetPropertyKeys(this->GetPropertyKeys());
      result = prop3D->HasTranslucentPolygonalGeometry();
    }
  }

  return result;
}

// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int svtkAssembly::RenderVolumetricGeometry(svtkViewport* ren)
{
  this->UpdatePaths();

  // for allocating render time between components
  // simple equal allocation
  double fraction =
    this->AllocatedRenderTime / static_cast<double>(this->Paths->GetNumberOfItems());

  int renderedSomething = 0;

  // render the Paths
  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility())
    {
      prop3D->SetPropertyKeys(this->GetPropertyKeys());
      prop3D->SetAllocatedRenderTime(fraction, ren);
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop3D->RenderVolumetricGeometry(ren);
      prop3D->PokeMatrix(nullptr);
    }
  }

  return (renderedSomething > 0) ? 1 : 0;
}

// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int svtkAssembly::RenderOpaqueGeometry(svtkViewport* ren)
{
  this->UpdatePaths();

  // for allocating render time between components
  // simple equal allocation
  double fraction =
    this->AllocatedRenderTime / static_cast<double>(this->Paths->GetNumberOfItems());

  int renderedSomething = 0;

  // render the Paths
  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility())
    {
      prop3D->SetPropertyKeys(this->GetPropertyKeys());
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      prop3D->SetAllocatedRenderTime(fraction, ren);
      renderedSomething += prop3D->RenderOpaqueGeometry(ren);
      prop3D->PokeMatrix(nullptr);
    }
  }

  return (renderedSomething > 0) ? 1 : 0;
}

void svtkAssembly::ReleaseGraphicsResources(svtkWindow* renWin)
{
  svtkProp3D* prop3D;

  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (prop3D = this->Parts->GetNextProp3D(pit));)
  {
    prop3D->ReleaseGraphicsResources(renWin);
  }
}

void svtkAssembly::GetActors(svtkPropCollection* ac)
{
  this->UpdatePaths();

  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    svtkActor* actor = svtkActor::SafeDownCast(prop3D);
    if (actor)
    {
      ac->AddItem(actor);
    }
  }
}

void svtkAssembly::GetVolumes(svtkPropCollection* ac)
{
  this->UpdatePaths();

  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    svtkVolume* volume = svtkVolume::SafeDownCast(prop3D);
    if (volume)
    {
      ac->AddItem(volume);
    }
  }
}

void svtkAssembly::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

// Return the next part in the hierarchy of assembly Parts.  This method
// returns a properly transformed and updated actor.
svtkAssemblyPath* svtkAssembly::GetNextPath()
{
  return this->Paths ? this->Paths->GetNextItem() : nullptr;
}

int svtkAssembly::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}

// Build the assembly paths if necessary. UpdatePaths()
// is only called when the assembly is at the root
// of the hierarchy; otherwise UpdatePaths() is called.
void svtkAssembly::UpdatePaths()
{
  if (this->GetMTime() > this->PathTime ||
    (this->Paths != nullptr && this->Paths->GetMTime() > this->PathTime))
  {
    if (this->Paths)
    {
      this->Paths->Delete();
      this->Paths = nullptr;
    }

    // Create the list to hold all the paths
    this->Paths = svtkAssemblyPaths::New();
    svtkAssemblyPath* path = svtkAssemblyPath::New();

    // add ourselves to the path to start things off
    path->AddNode(this, this->GetMatrix());

    // Add nodes as we proceed down the hierarchy
    svtkProp3D* prop3D;
    svtkCollectionSimpleIterator pit;
    for (this->Parts->InitTraversal(pit); (prop3D = this->Parts->GetNextProp3D(pit));)
    {
      path->AddNode(prop3D, prop3D->GetMatrix());

      // dive into the hierarchy
      prop3D->BuildPaths(this->Paths, path);

      // when returned, pop the last node off of the
      // current path
      path->DeleteLastNode();
    }

    path->Delete();
    this->PathTime.Modified();
  }
}

// Build assembly paths from this current assembly. A path consists of
// an ordered sequence of props, with transformations properly concatenated.
void svtkAssembly::BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path)
{
  svtkProp3D* prop3D;

  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (prop3D = this->Parts->GetNextProp3D(pit));)
  {
    path->AddNode(prop3D, prop3D->GetMatrix());

    // dive into the hierarchy
    prop3D->BuildPaths(paths, path);

    // when returned, pop the last node off of the
    // current path
    path->DeleteLastNode();
  }
}

// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkAssembly::GetBounds()
{
  this->UpdatePaths();

  // now calculate the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = SVTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -SVTK_DOUBLE_MAX;

  int propVisible = 0;

  svtkAssemblyPath* path;
  svtkCollectionSimpleIterator sit;
  for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
  {
    svtkProp3D* prop3D = static_cast<svtkProp3D*>(path->GetLastNode()->GetViewProp());
    if (prop3D->GetVisibility() && prop3D->GetUseBounds())
    {
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      const double* bounds = prop3D->GetBounds();
      prop3D->PokeMatrix(nullptr);

      // Skip any props that have uninitialized bounds
      if (bounds == nullptr || !svtkMath::AreBoundsInitialized(bounds))
      {
        continue;
      }
      // Only set the prop as visible if at least one prop has a valid bounds
      propVisible = 1;

      double bbox[24];
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

      for (int i = 0; i < 8; i++)
      {
        for (int n = 0; n < 3; n++)
        {
          if (bbox[i * 3 + n] < this->Bounds[n * 2])
          {
            this->Bounds[n * 2] = bbox[i * 3 + n];
          }
          if (bbox[i * 3 + n] > this->Bounds[n * 2 + 1])
          {
            this->Bounds[n * 2 + 1] = bbox[i * 3 + n];
          }
        } // for each coordinate axis
      }   // for each point of box
    }     // if visible && prop3d
  }       // for each path

  if (!propVisible)
  {
    svtkMath::UninitializeBounds(this->Bounds);
  }

  return this->Bounds;
}

svtkMTimeType svtkAssembly::GetMTime()
{
  svtkMTimeType mTime = this->svtkProp3D::GetMTime();
  svtkProp3D* prop;

  svtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); (prop = this->Parts->GetNextProp3D(pit));)
  {
    svtkMTimeType time = prop->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

void svtkAssembly::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems() << " parts in this assembly\n";
}
