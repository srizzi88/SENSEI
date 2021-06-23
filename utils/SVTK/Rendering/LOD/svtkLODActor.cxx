/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLODActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLODActor.h"

#include "svtkMapperCollection.h"
#include "svtkMaskPoints.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"
#include "svtkTimerLog.h"

#include <cmath>

svtkStandardNewMacro(svtkLODActor);
svtkCxxSetObjectMacro(svtkLODActor, LowResFilter, svtkPolyDataAlgorithm);
svtkCxxSetObjectMacro(svtkLODActor, MediumResFilter, svtkPolyDataAlgorithm);

//----------------------------------------------------------------------------
svtkLODActor::svtkLODActor()
{
  // get a hardware dependent actor and mappers
  this->Device = svtkActor::New();
  svtkMatrix4x4* m = svtkMatrix4x4::New();
  this->Device->SetUserMatrix(m);
  m->Delete();

  this->LODMappers = svtkMapperCollection::New();
  this->MediumResFilter = nullptr;
  this->LowResFilter = nullptr;
  this->NumberOfCloudPoints = 150;
  this->LowMapper = nullptr;
  this->MediumMapper = nullptr;
}

//----------------------------------------------------------------------------
svtkLODActor::~svtkLODActor()
{
  this->Device->Delete();
  this->Device = nullptr;
  this->DeleteOwnLODs();
  this->LODMappers->Delete();
}

//----------------------------------------------------------------------------
void svtkLODActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cloud Points: " << this->NumberOfCloudPoints << endl;

  // how should we print out the LODMappers?
  os << indent << "Number Of LOD Mappers: " << this->LODMappers->GetNumberOfItems() << endl;
  os << indent << "Medium Resolution Filter: " << this->MediumResFilter << "\n";
  if (this->MediumResFilter)
  {
    this->MediumResFilter->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Low Resolution Filter: " << this->LowResFilter << "\n";
  if (this->LowResFilter)
  {
    this->LowResFilter->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void svtkLODActor::Render(svtkRenderer* ren, svtkMapper* svtkNotUsed(m))
{
  float myTime, bestTime, tempTime;
  svtkMatrix4x4* matrix;
  svtkMapper *mapper, *bestMapper;

  if (!this->Mapper)
  {
    svtkErrorMacro("No mapper for actor.");
    return;
  }

  // first time through create lods if non have been added
  if (this->LODMappers->GetNumberOfItems() == 0)
  {
    this->CreateOwnLODs();
  }

  // If the actor has changed or the primary mapper has changed ...
  // Is this the correct test?
  if (this->MediumMapper)
  {
    if (this->GetMTime() > this->BuildTime || this->Mapper->GetMTime() > this->BuildTime)
    {
      this->UpdateOwnLODs();
    }
  }

  // figure out how much time we have to render
  myTime = this->AllocatedRenderTime;

  // Figure out which resolution to use
  // none is a valid resolution. Do we want to have a lowest:
  // bbox, single point, ...
  // There is no order to the list, so it is assumed that mappers that take
  // longer to render are better quality.
  // Timings might become out of date, but we rely on

  bestMapper = this->Mapper;
  bestTime = bestMapper->GetTimeToDraw();
  if (bestTime > myTime)
  {
    svtkCollectionSimpleIterator mit;
    this->LODMappers->InitTraversal(mit);
    while ((mapper = this->LODMappers->GetNextMapper(mit)) != nullptr && bestTime != 0.0)
    {
      tempTime = mapper->GetTimeToDraw();

      // If the LOD has never been rendered, select it!
      if (tempTime == 0.0)
      {
        bestMapper = mapper;
        bestTime = 0.0;
      }
      else
      {
        if (bestTime > myTime && tempTime < bestTime)
        {
          bestMapper = mapper;
          bestTime = tempTime;
        }
        if (tempTime > bestTime && tempTime < myTime)
        {
          bestMapper = mapper;
          bestTime = tempTime;
        }
      }
    }
  }

  // render the property
  if (!this->Property)
  {
    // force creation of a property
    this->GetProperty();
  }
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
  {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
  }
  this->Device->SetProperty(this->Property);

  // render the texture
  if (this->Texture)
  {
    this->Texture->Render(ren);
  }

  // make sure the device has the same matrix
  matrix = this->Device->GetUserMatrix();
  this->GetMatrix(matrix);

  // Store information on time it takes to render.
  // We might want to estimate time from the number of polygons in mapper.

  // The internal actor needs to share property keys. This allows depth peeling
  // etc to work.
  this->Device->SetPropertyKeys(this->GetPropertyKeys());

  this->Device->Render(ren, bestMapper);
  this->EstimatedRenderTime = bestMapper->GetTimeToDraw();
}

int svtkLODActor::RenderOpaqueGeometry(svtkViewport* vp)
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

  // is this actor opaque ?
  // Do this check only when not in selection mode
  if (this->GetIsOpaque() || (ren->GetSelector() && this->Property->GetOpacity() > 0.0))
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
    }
    this->Render(ren, this->Mapper);

    renderedSomething = 1;
  }

  return renderedSomething;
}

void svtkLODActor::ReleaseGraphicsResources(svtkWindow* renWin)
{
  svtkMapper* mapper;
  svtkActor::ReleaseGraphicsResources(renWin);

  // broadcast the message down to the individual LOD mappers
  svtkCollectionSimpleIterator mit;
  for (this->LODMappers->InitTraversal(mit); (mapper = this->LODMappers->GetNextMapper(mit));)
  {
    mapper->ReleaseGraphicsResources(renWin);
  }
}

//----------------------------------------------------------------------------
// does not matter if mapper is in mapper collection.
void svtkLODActor::AddLODMapper(svtkMapper* mapper)
{
  if (this->MediumMapper)
  {
    this->DeleteOwnLODs();
  }

  if (!this->Mapper)
  {
    this->SetMapper(mapper);
  }

  this->LODMappers->AddItem(mapper);
}

//----------------------------------------------------------------------------
// Can only be used if no LOD mappers have been added.
// Maybe we should remove this exculsive feature.
void svtkLODActor::CreateOwnLODs()
{
  if (this->MediumMapper)
  {
    return;
  }

  if (!this->Mapper)
  {
    svtkErrorMacro("Cannot create LODs with out a mapper.");
    return;
  }

  // There are ways of getting around this limitation...
  if (this->LODMappers->GetNumberOfItems() > 0)
  {
    svtkErrorMacro(<< "Cannot generate LOD mappers when some have been added already");
    return;
  }

  // create filters and mappers
  if (!this->MediumResFilter)
  {
    svtkMaskPoints* mediumResFilter = svtkMaskPoints::New();
    mediumResFilter->RandomModeOn();
    mediumResFilter->GenerateVerticesOn();
    this->SetMediumResFilter(mediumResFilter);
    mediumResFilter->Delete();
  }

  this->MediumMapper = svtkPolyDataMapper::New();

  if (!this->LowResFilter)
  {
    svtkOutlineFilter* lowResFilter = svtkOutlineFilter::New();
    this->SetLowResFilter(lowResFilter);
    lowResFilter->Delete();
  }

  this->LowMapper = svtkPolyDataMapper::New();
  this->LODMappers->AddItem(this->MediumMapper);
  this->LODMappers->AddItem(this->LowMapper);

  this->UpdateOwnLODs();
}

//----------------------------------------------------------------------------
void svtkLODActor::UpdateOwnLODs()
{
  if (!this->Mapper)
  {
    svtkErrorMacro("Cannot create LODs with out a mapper.");
    return;
  }

  if (!this->MediumMapper)
  {
    this->CreateOwnLODs();
    if (!this->MediumMapper)
    { // could not create the LODs
      return;
    }
  }

  // connect the filters to the mapper, and set parameters
  this->MediumResFilter->SetInputConnection(this->Mapper->GetInputConnection(0, 0));
  this->LowResFilter->SetInputConnection(this->Mapper->GetInputConnection(0, 0));

  // If the medium res filter is a svtkMaskPoints, then set the ivar in here.
  // In reality, we should deprecate the svtkLODActor::SetNumberOfCloudPoints
  // method, since now you can get the filters that make up the low and
  // medium res and set them yourself.
  if (svtkMaskPoints* f = svtkMaskPoints::SafeDownCast(this->MediumResFilter))
  {
    f->SetMaximumNumberOfPoints(this->NumberOfCloudPoints);
  }

  // copy all parameters including LUTs, scalar range, etc.
  this->MediumMapper->ShallowCopy(this->Mapper);
  this->MediumMapper->SetInputConnection(this->MediumResFilter->GetOutputPort());
  this->LowMapper->ShallowCopy(this->Mapper);
  this->LowMapper->ScalarVisibilityOff();
  this->LowMapper->SetInputConnection(this->LowResFilter->GetOutputPort());

  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
// Deletes Mappers and filters created by this object.
// (number two and three)
void svtkLODActor::DeleteOwnLODs()
{
  // remove the mappers from the LOD collection
  if (this->LowMapper)
  {
    this->LODMappers->RemoveItem(this->LowMapper);
    this->LowMapper->Delete();
    this->LowMapper = nullptr;
  }

  if (this->MediumMapper)
  {
    this->LODMappers->RemoveItem(this->MediumMapper);
    this->MediumMapper->Delete();
    this->MediumMapper = nullptr;
  }

  // delete the filters used to create the LODs ...
  // The nullptr check should not be necessary, but for sanity ...
  this->SetLowResFilter(nullptr);
  this->SetMediumResFilter(nullptr);
}

//----------------------------------------------------------------------------
void svtkLODActor::Modified()
{
  if (this->Device) // Will be nullptr only during destruction of this class.
  {
    this->Device->Modified();
  }
  this->svtkActor::Modified();
}

void svtkLODActor::ShallowCopy(svtkProp* prop)
{
  svtkLODActor* a = svtkLODActor::SafeDownCast(prop);
  if (a)
  {
    this->SetNumberOfCloudPoints(a->GetNumberOfCloudPoints());
    svtkMapperCollection* c = a->GetLODMappers();
    svtkMapper* map;
    svtkCollectionSimpleIterator mit;
    for (c->InitTraversal(mit); (map = c->GetNextMapper(mit));)
    {
      this->AddLODMapper(map);
    }
  }

  // Now do superclass
  this->svtkActor::ShallowCopy(prop);
}
