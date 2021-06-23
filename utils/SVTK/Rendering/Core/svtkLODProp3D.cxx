/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLODProp3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLODProp3D.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkImageMapper3D.h"
#include "svtkImageSlice.h"
#include "svtkLinearTransform.h"
#include "svtkMapper.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPropCollection.h"
#include "svtkVolume.h"

#include <cmath>

svtkStandardNewMacro(svtkLODProp3D);

#define SVTK_INDEX_NOT_IN_USE (-1)

#define SVTK_INVALID_LOD_INDEX (-2)

#define SVTK_LOD_ACTOR_TYPE 1
#define SVTK_LOD_VOLUME_TYPE 2
#define SVTK_LOD_IMAGE_TYPE 3

class svtkLODProp3DCallback : public svtkCommand
{
public:
  // generic new method
  static svtkLODProp3DCallback* New() { return new svtkLODProp3DCallback; }

  // the execute
  void Execute(svtkObject* caller, unsigned long event, void* svtkNotUsed(v)) override
  {
    svtkProp* po = svtkProp::SafeDownCast(caller);
    if (event == svtkCommand::PickEvent && po)
    {
      this->Self->InvokeEvent(svtkCommand::PickEvent, nullptr);
    }
  }

  // some ivars that should be set
  svtkLODProp3D* Self;
};

// Construct a new svtkLODProp3D. Automatic LOD selection is on, there are
// no LODs.
svtkLODProp3D::svtkLODProp3D()
{
  this->NumberOfEntries = 0;
  this->NumberOfLODs = 0;
  this->LODs = nullptr;
  this->CurrentIndex = 1000;
  this->AutomaticLODSelection = 1;
  this->SelectedLODID = 1000;
  this->SelectedLODIndex = -1;
  this->SelectedPickLODID = 1000;
  this->AutomaticPickLODSelection = 1;
  this->PickCallback = svtkLODProp3DCallback::New();
  this->PickCallback->Self = this;
}

// Destruct the svtkLODProp3D. Delete the svtkProp3Ds that were created
// for the LODs. Delete the array of LODs.
svtkLODProp3D::~svtkLODProp3D()
{
  // Delete the svtkProp3D objects that were created
  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE)
    {
      this->LODs[i].Prop3D->RemoveConsumer(this);
      this->LODs[i].Prop3D->RemoveObserver(this->PickCallback);
      this->LODs[i].Prop3D->Delete();
    }
  }

  // Delete the array of LODs
  if (this->NumberOfEntries > 0)
  {
    delete[] this->LODs;
  }

  this->PickCallback->Delete();
}

int svtkLODProp3D::ConvertIDToIndex(int id)
{
  int index = 0;

  while (index < this->NumberOfEntries && this->LODs[index].ID != id)
  {
    index++;
  }
  if (index == this->NumberOfEntries)
  {
    svtkErrorMacro(<< "Could not locate ID: " << id);
    index = SVTK_INVALID_LOD_INDEX;
  }

  return index;
}

// Get the next available entry index
int svtkLODProp3D::GetNextEntryIndex()
{
  // Search for an available index
  int index = 0;
  while (index < this->NumberOfEntries && this->LODs[index].ID != SVTK_INDEX_NOT_IN_USE)
  {
    index++;
  }

  // If an available index was not found, we need more entries
  if (index >= this->NumberOfEntries)
  {
    // If we have no entries, create 10. If we already have some, create
    // twice as many as we already have.
    int amount = (this->NumberOfEntries) ? (this->NumberOfEntries * 2) : (10);

    // Make the new array
    svtkLODProp3DEntry* newLODs = new svtkLODProp3DEntry[amount];

    int i;
    // Copy the old entries into the new array
    for (i = 0; i < this->NumberOfEntries; i++)
    {
      newLODs[i].Prop3D = this->LODs[i].Prop3D;
      newLODs[i].Prop3DType = this->LODs[i].Prop3DType;
      newLODs[i].ID = this->LODs[i].ID;
      newLODs[i].EstimatedTime = this->LODs[i].EstimatedTime;
      newLODs[i].Level = this->LODs[i].Level;
      newLODs[i].State = this->LODs[i].State;
    }

    // This is the index that we will return - one past the old entries
    index = i;

    // Initialize the new entries to default values
    for (; i < amount; i++)
    {
      newLODs[i].Prop3D = nullptr;
      newLODs[i].ID = SVTK_INDEX_NOT_IN_USE;
    }

    // Delete the old array and set the pointer to the new one
    delete[] this->LODs;
    this->LODs = newLODs;

    // Set the new number of entries that we have
    this->NumberOfEntries = amount;
  }
  return index;
}

// Get the bounds of this prop. This is just the max bounds of all LODs
double* svtkLODProp3D::GetBounds()
{
  double newBounds[6];
  int first = 1;

  // Loop through all valid entries
  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE)
    {
      svtkProp3D* p = this->LODs[i].Prop3D;
      if (p->GetMTime() < this->GetMTime())
      {
        p->SetUserMatrix(this->GetMatrix());
      }

      // Get the bounds of this entry
      p->GetBounds(newBounds);

      // If this is the first entry, this is the current bounds
      if (first)
      {
        memcpy(this->Bounds, newBounds, 6 * sizeof(double));
        first = 0;
      }
      // If this is not the first entry, compare these bounds with the
      // current bounds expanding the current ones as necessary
      else
      {
        this->Bounds[0] = (newBounds[0] < this->Bounds[0]) ? (newBounds[0]) : (this->Bounds[0]);
        this->Bounds[1] = (newBounds[1] > this->Bounds[1]) ? (newBounds[1]) : (this->Bounds[1]);
        this->Bounds[2] = (newBounds[2] < this->Bounds[2]) ? (newBounds[2]) : (this->Bounds[2]);
        this->Bounds[3] = (newBounds[3] > this->Bounds[3]) ? (newBounds[3]) : (this->Bounds[3]);
        this->Bounds[4] = (newBounds[4] < this->Bounds[4]) ? (newBounds[4]) : (this->Bounds[4]);
        this->Bounds[5] = (newBounds[5] > this->Bounds[5]) ? (newBounds[5]) : (this->Bounds[5]);
      }
    }
  }

  return this->Bounds;
}

// Method to remove a LOD based on an ID
void svtkLODProp3D::RemoveLOD(int id)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  this->LODs[index].Prop3D->RemoveConsumer(this);
  this->LODs[index].Prop3D->RemoveObserver(this->PickCallback);
  this->LODs[index].Prop3D->Delete();
  this->LODs[index].ID = SVTK_INDEX_NOT_IN_USE;
  this->NumberOfLODs--;
}

// Convenience method to get the ID of the LOD that was used
// during the last render
int svtkLODProp3D::GetLastRenderedLODID()
{
  // Check if the selected index is in range
  if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
  {
    return -1;
  }

  // Check if the selected index is valid
  if (this->LODs[this->SelectedLODIndex].ID == SVTK_INDEX_NOT_IN_USE)
  {
    return -1;
  }

  return this->LODs[this->SelectedLODIndex].ID;
}

// Convenience method to get the estimated render time for a given LOD
// based on an ID (the number returned when the LOD was added)
double svtkLODProp3D::GetLODEstimatedRenderTime(int id)
{
  int index = this->ConvertIDToIndex(id);

  return (index != SVTK_INVALID_LOD_INDEX) ? this->GetLODIndexEstimatedRenderTime(index) : 0.0;
}

double svtkLODProp3D::GetLODIndexEstimatedRenderTime(int index)
{
  return (index < 0 || index >= this->NumberOfEntries) ? 0 : this->LODs[index].EstimatedTime;
}

// Convenience method to set an actor LOD without a texture, or a
// backface property.  Needed from wrappers where null pointers
// are not possible
int svtkLODProp3D::AddLOD(svtkMapper* m, svtkProperty* p, double time)
{
  return this->AddLOD(m, p, nullptr, nullptr, time);
}

// Convenience method to set an actor LOD without a texture.
// Needed from wrappers where null pointers are not possible
int svtkLODProp3D::AddLOD(svtkMapper* m, svtkProperty* p, svtkProperty* back, double time)
{
  return this->AddLOD(m, p, back, nullptr, time);
}

// Convenience method to set an actor LOD without a backface property.
// Needed from wrappers where null pointers are not possible
int svtkLODProp3D::AddLOD(svtkMapper* m, svtkProperty* p, svtkTexture* t, double time)
{
  return this->AddLOD(m, p, nullptr, t, time);
}

// Convenience method to set an actor LOD without a property.
// Needed from wrappers where null pointers are not possible
int svtkLODProp3D::AddLOD(svtkMapper* m, svtkTexture* t, double time)
{
  return this->AddLOD(m, nullptr, nullptr, t, time);
}

// Convenience method to set an actor LOD without a texture or a property.
// Needed from wrappers where null pointers are not possible
int svtkLODProp3D::AddLOD(svtkMapper* m, double time)
{
  return this->AddLOD(m, nullptr, nullptr, nullptr, time);
}

// The real method for adding an actor LOD.
int svtkLODProp3D::AddLOD(
  svtkMapper* m, svtkProperty* p, svtkProperty* back, svtkTexture* t, double time)
{
  int index = this->GetNextEntryIndex();
  svtkActor* actor = svtkActor::New();
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  this->GetMatrix(matrix);
  actor->SetUserMatrix(matrix);
  matrix->Delete();
  actor->SetMapper(m);
  if (p)
  {
    actor->SetProperty(p);
  }

  if (back)
  {
    actor->SetBackfaceProperty(back);
  }

  if (t)
  {
    actor->SetTexture(t);
  }

  actor->AddConsumer(this);

  this->LODs[index].Prop3D = actor;
  this->LODs[index].Prop3DType = SVTK_LOD_ACTOR_TYPE;
  this->LODs[index].ID = this->CurrentIndex++;
  this->LODs[index].EstimatedTime = time;
  this->LODs[index].Level = 0.0;
  this->LODs[index].State = 1;
  this->LODs[index].Prop3D->AddObserver(svtkCommand::PickEvent, this->PickCallback);
  this->NumberOfLODs++;

  actor->SetEstimatedRenderTime(time);

  return this->LODs[index].ID;
}

// Convenience method to set a volume LOD without a property.
// Needed from wrappers where null pointers are not possible
int svtkLODProp3D::AddLOD(svtkAbstractVolumeMapper* m, double time)
{
  return this->AddLOD(m, nullptr, time);
}

// The real method for adding a volume LOD.
int svtkLODProp3D::AddLOD(svtkAbstractVolumeMapper* m, svtkVolumeProperty* p, double time)
{
  int index = this->GetNextEntryIndex();
  svtkVolume* volume = svtkVolume::New();
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  this->GetMatrix(matrix);
  volume->SetUserMatrix(matrix);
  matrix->Delete();
  volume->SetMapper(m);
  if (p)
  {
    volume->SetProperty(p);
  }

  volume->AddConsumer(this);

  this->LODs[index].Prop3D = volume;
  this->LODs[index].Prop3DType = SVTK_LOD_VOLUME_TYPE;
  this->LODs[index].ID = this->CurrentIndex++;
  this->LODs[index].EstimatedTime = time;
  this->LODs[index].Level = 0.0;
  this->LODs[index].State = 1;
  this->LODs[index].Prop3D->AddObserver(svtkCommand::PickEvent, this->PickCallback);
  this->NumberOfLODs++;

  volume->SetEstimatedRenderTime(time);

  return this->LODs[index].ID;
}

// Convenience method to set a volume LOD without a property.
// Needed from wrappers where null pointers are not possible
int svtkLODProp3D::AddLOD(svtkImageMapper3D* m, double time)
{
  return this->AddLOD(m, nullptr, time);
}

// The real method for adding a volume LOD.
int svtkLODProp3D::AddLOD(svtkImageMapper3D* m, svtkImageProperty* p, double time)
{
  int index = this->GetNextEntryIndex();
  svtkImageSlice* image = svtkImageSlice::New();
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  this->GetMatrix(matrix);
  image->SetUserMatrix(matrix);
  matrix->Delete();
  image->SetMapper(m);
  if (p)
  {
    image->SetProperty(p);
  }

  image->AddConsumer(this);

  this->LODs[index].Prop3D = image;
  this->LODs[index].Prop3DType = SVTK_LOD_IMAGE_TYPE;
  this->LODs[index].ID = this->CurrentIndex++;
  this->LODs[index].EstimatedTime = time;
  this->LODs[index].Level = 0.0;
  this->LODs[index].State = 1;
  this->LODs[index].Prop3D->AddObserver(svtkCommand::PickEvent, this->PickCallback);
  this->NumberOfLODs++;

  image->SetEstimatedRenderTime(time);

  return this->LODs[index].ID;
}

// Set the mapper for an LOD that is an actor
void svtkLODProp3D::SetLODMapper(int id, svtkMapper* m)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set an actor mapper on a non-actor!");
    return;
  }

  static_cast<svtkActor*>(this->LODs[index].Prop3D)->SetMapper(m);
}

// Get the mapper for an LOD that is an actor
void svtkLODProp3D::GetLODMapper(int id, svtkMapper** m)
{
  *m = nullptr;

  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get an actor mapper on a non-actor!");

    return;
  }

  *m = static_cast<svtkActor*>(this->LODs[index].Prop3D)->GetMapper();
}

// Set the mapper for an LOD that is a volume
void svtkLODProp3D::SetLODMapper(int id, svtkAbstractVolumeMapper* m)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_VOLUME_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set a volume mapper on a non-volume!");
    return;
  }

  static_cast<svtkVolume*>(this->LODs[index].Prop3D)->SetMapper(m);
}

// Get the mapper for an LOD that is an actor
void svtkLODProp3D::GetLODMapper(int id, svtkAbstractVolumeMapper** m)
{
  *m = nullptr;

  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_VOLUME_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get a volume mapper on a non-volume!");
    return;
  }

  *m = static_cast<svtkVolume*>(this->LODs[index].Prop3D)->GetMapper();
}

// Set the mapper for an LOD that is an image
void svtkLODProp3D::SetLODMapper(int id, svtkImageMapper3D* m)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_IMAGE_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set an image mapper on a non-image!");
    return;
  }

  static_cast<svtkImageSlice*>(this->LODs[index].Prop3D)->SetMapper(m);
}

// Get the mapper for an LOD that is an image
void svtkLODProp3D::GetLODMapper(int id, svtkImageMapper3D** m)
{
  *m = nullptr;

  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_IMAGE_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get an image mapper on a non-image!");
    return;
  }

  *m = static_cast<svtkImageSlice*>(this->LODs[index].Prop3D)->GetMapper();
}

// Get the mapper for an LOD that is an AbstractMapper3D
svtkAbstractMapper3D* svtkLODProp3D::GetLODMapper(int id)
{
  svtkAbstractMapper3D* m = nullptr;

  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return m;
  }

  if (this->LODs[index].Prop3DType == SVTK_LOD_ACTOR_TYPE)
  {
    m = static_cast<svtkActor*>(this->LODs[index].Prop3D)->GetMapper();
  }
  else if (this->LODs[index].Prop3DType == SVTK_LOD_VOLUME_TYPE)
  {
    m = static_cast<svtkVolume*>(this->LODs[index].Prop3D)->GetMapper();
  }
  else if (this->LODs[index].Prop3DType == SVTK_LOD_IMAGE_TYPE)
  {
    m = static_cast<svtkImageSlice*>(this->LODs[index].Prop3D)->GetMapper();
  }

  return m;
}

// Set the property for an LOD that is an actor
void svtkLODProp3D::SetLODProperty(int id, svtkProperty* p)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set an actor property on a non-actor!");
    return;
  }

  static_cast<svtkActor*>(this->LODs[index].Prop3D)->SetProperty(p);
}

// Get the property for an LOD that is an actor
void svtkLODProp3D::GetLODProperty(int id, svtkProperty** p)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get an actor property on a non-actor!");
    return;
  }

  *p = static_cast<svtkActor*>(this->LODs[index].Prop3D)->GetProperty();
}

// Set the property for an LOD that is a volume
void svtkLODProp3D::SetLODProperty(int id, svtkVolumeProperty* p)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_VOLUME_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set a volume property on a non-volume!");
    return;
  }

  static_cast<svtkVolume*>(this->LODs[index].Prop3D)->SetProperty(p);
}

// Get the property for an LOD that is an actor
void svtkLODProp3D::GetLODProperty(int id, svtkVolumeProperty** p)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_VOLUME_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get a volume property on a non-volume!");
    return;
  }

  *p = static_cast<svtkVolume*>(this->LODs[index].Prop3D)->GetProperty();
}

// Set the property for an LOD that is an image
void svtkLODProp3D::SetLODProperty(int id, svtkImageProperty* p)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_IMAGE_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set an image property on a non-image!");
    return;
  }

  static_cast<svtkImageSlice*>(this->LODs[index].Prop3D)->SetProperty(p);
}

// Get the property for an LOD that is an image
void svtkLODProp3D::GetLODProperty(int id, svtkImageProperty** p)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_IMAGE_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get an image property on a non-image!");
    return;
  }

  *p = static_cast<svtkImageSlice*>(this->LODs[index].Prop3D)->GetProperty();
}

// Set the texture for an LOD that is an actor
void svtkLODProp3D::SetLODTexture(int id, svtkTexture* t)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set an actor texture on a non-actor!");
    return;
  }

  static_cast<svtkActor*>(this->LODs[index].Prop3D)->SetTexture(t);
}

// Get the texture for an LOD that is an actor
void svtkLODProp3D::GetLODTexture(int id, svtkTexture** t)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get an actor texture on a non-actor!");
    return;
  }

  *t = static_cast<svtkActor*>(this->LODs[index].Prop3D)->GetTexture();
}

// Set the backface property for an LOD that is an actor
void svtkLODProp3D::SetLODBackfaceProperty(int id, svtkProperty* t)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot set an actor backface property on a non-actor!");
    return;
  }

  static_cast<svtkActor*>(this->LODs[index].Prop3D)->SetBackfaceProperty(t);
}

// Get the backface property for an LOD that is an actor
void svtkLODProp3D::GetLODBackfaceProperty(int id, svtkProperty** t)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (this->LODs[index].Prop3DType != SVTK_LOD_ACTOR_TYPE)
  {
    svtkErrorMacro(<< "Error: Cannot get an actor backface property on a non-actor!");
    return;
  }

  *t = static_cast<svtkActor*>(this->LODs[index].Prop3D)->GetBackfaceProperty();
}

void svtkLODProp3D::EnableLOD(int id)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX || index == SVTK_INDEX_NOT_IN_USE)
  {
    return;
  }

  this->LODs[index].State = 1;
}

void svtkLODProp3D::DisableLOD(int id)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX || index == SVTK_INDEX_NOT_IN_USE)
  {
    return;
  }

  this->LODs[index].State = 0;
}

int svtkLODProp3D::IsLODEnabled(int id)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX || index == SVTK_INDEX_NOT_IN_USE)
  {
    return 0;
  }

  return this->LODs[index].State;
}

void svtkLODProp3D::SetLODLevel(int id, double level)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX || index == SVTK_INDEX_NOT_IN_USE)
  {
    return;
  }

  this->LODs[index].Level = level;
}

double svtkLODProp3D::GetLODLevel(int id)
{
  int index = this->ConvertIDToIndex(id);

  if (index == SVTK_INVALID_LOD_INDEX || index == SVTK_INDEX_NOT_IN_USE)
  {
    return -1;
  }

  return this->LODs[index].Level;
}

double svtkLODProp3D::GetLODIndexLevel(int index)
{
  if (index == SVTK_INVALID_LOD_INDEX || index == SVTK_INDEX_NOT_IN_USE)
  {
    return -1;
  }
  return this->LODs[index].Level;
}

// Release any graphics resources that any of the LODs might be using
// for a particular window (such as display lists).
void svtkLODProp3D::ReleaseGraphicsResources(svtkWindow* w)
{
  // Loop through all LODs and pass this message along
  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE)
    {
      this->LODs[i].Prop3D->ReleaseGraphicsResources(w);
    }
  }
}

// Standard render method - render any opaque geometry in the selected LOD
int svtkLODProp3D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  // Check if the selected index is in range
  if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
  {
    svtkErrorMacro(<< "Index out of range!");
    return 0;
  }

  // Check if the selected index is valid
  if (this->LODs[this->SelectedLODIndex].ID == SVTK_INDEX_NOT_IN_USE)
  {
    svtkErrorMacro(<< "Index not valid!");
    return 0;
  }

  // Actually do the rendering
  this->UpdateKeysForSelectedProp();
  int retval = this->LODs[this->SelectedLODIndex].Prop3D->RenderOpaqueGeometry(viewport);

  this->EstimatedRenderTime += this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime();

  return retval;
}

// Standard render method - render any translucent geometry in the selected LOD
int svtkLODProp3D::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  // Check if the selected index is in range
  if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
  {
    svtkErrorMacro(<< "Index out of range!");
    return 0;
  }

  // Check if the selected index is valid
  if (this->LODs[this->SelectedLODIndex].ID == SVTK_INDEX_NOT_IN_USE)
  {
    svtkErrorMacro(<< "Index not valid!");
    return 0;
  }

  // Actually do the rendering
  this->UpdateKeysForSelectedProp();
  int retval =
    this->LODs[this->SelectedLODIndex].Prop3D->RenderTranslucentPolygonalGeometry(viewport);

  this->EstimatedRenderTime += this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime();

  return retval;
}

// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkLODProp3D::HasTranslucentPolygonalGeometry()
{
  // Check if the selected index is in range
  if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
  {
    svtkErrorMacro(<< "Index out of range!");
    return 0;
  }

  // Check if the selected index is valid
  if (this->LODs[this->SelectedLODIndex].ID == SVTK_INDEX_NOT_IN_USE)
  {
    svtkErrorMacro(<< "Index not valid!");
    return 0;
  }

  return this->LODs[this->SelectedLODIndex].Prop3D->HasTranslucentPolygonalGeometry();
}

// Standard render method - render any translucent geometry in the selected LOD
int svtkLODProp3D::RenderVolumetricGeometry(svtkViewport* viewport)
{
  // Check if the selected index is in range
  if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
  {
    svtkErrorMacro(<< "Index out of range!");
    return 0;
  }

  // Check if the selected index is valid
  if (this->LODs[this->SelectedLODIndex].ID == SVTK_INDEX_NOT_IN_USE)
  {
    svtkErrorMacro(<< "Index not valid!");
    return 0;
  }

  // Actually do the rendering
  this->UpdateKeysForSelectedProp();
  int retval = this->LODs[this->SelectedLODIndex].Prop3D->RenderVolumetricGeometry(viewport);

  this->EstimatedRenderTime += this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime();

  return retval;
}

// Override the method from svtkProp - add to both this prop and the prop of
// the selected LOD
void svtkLODProp3D::AddEstimatedRenderTime(double t, svtkViewport* vp)
{
  // Add to this prop's estimated render time
  this->EstimatedRenderTime += t;

  // Check if the selected index is in range
  if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
  {
    svtkErrorMacro(<< "Index out of range!");
    return;
  }

  // Check if the selected index is valid
  if (this->LODs[this->SelectedLODIndex].ID == SVTK_INDEX_NOT_IN_USE)
  {
    svtkErrorMacro(<< "Index not valid!");
    return;
  }

  // Now that error checking is done, add to the estimated render time
  // of the selected LOD
  this->LODs[this->SelectedLODIndex].Prop3D->AddEstimatedRenderTime(t, vp);
}

void svtkLODProp3D::RestoreEstimatedRenderTime()
{
  // restore the EstimatedTime of the last LOD to be rendered
  if (this->SelectedLODIndex >= 0 && this->SelectedLODIndex < this->NumberOfEntries)
  {
    this->LODs[this->SelectedLODIndex].Prop3D->RestoreEstimatedRenderTime();
  }
}

// Set the allocated render time - this is where the decision is made
// as to which LOD to select
void svtkLODProp3D::SetAllocatedRenderTime(double t, svtkViewport* vp)
{
  int i;
  int index = -1;
  double bestTime;
  double bestLevel = 0;
  double targetTime;
  double estimatedTime;
  double newTime;

  // update the EstimatedTime of the last LOD to be rendered
  if (this->SelectedLODIndex >= 0 && this->SelectedLODIndex < this->NumberOfEntries &&
    this->LODs[this->SelectedLODIndex].ID != SVTK_INDEX_NOT_IN_USE)
  {
    // For stability, blend in the new time - 25% old + 75% new
    newTime = this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime(vp);
    this->LODs[this->SelectedLODIndex].EstimatedTime =
      0.25 * this->LODs[this->SelectedLODIndex].EstimatedTime + 0.75 * newTime;
  }

  this->SavedEstimatedRenderTime = this->EstimatedRenderTime;

  if (this->AutomaticLODSelection)
  {
    bestTime = -1.0;

    targetTime = t;

    for (i = 0; i < this->NumberOfEntries; i++)
    {
      if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE && this->LODs[i].State == 1)
      {
        // Gather some information
        estimatedTime = this->GetLODIndexEstimatedRenderTime(i);

        // If we've never rendered this LOD and we have no info on it,
        // then try it out
        if (estimatedTime == 0.0)
        {
          index = i;
          bestTime = 0.0;
          bestLevel = this->GetLODIndexLevel(i);
          break;
        }

        // If we do have at least a guess as to the render time, and
        // this seems like the best we have so far, pick it.
        // It is the best we have if
        //
        // 1) our estimated time is less than what we are looking for,
        //    but greater than any we have selected so far.
        //
        // 2) we have not selected anything else yet
        //    (regardless of what the estimated time is)
        //
        // 3) it is less than the time of the currently selected LOD
        //    if that LOD's time is greater than the time we are targeting.
        //
        if (estimatedTime > 0.0 &&
          ((estimatedTime > bestTime && estimatedTime < targetTime) || (bestTime == -1.0) ||
            (estimatedTime < bestTime && bestTime > targetTime)))
        {
          index = i;
          bestTime = estimatedTime;
          bestLevel = this->GetLODIndexLevel(i);
        }
      }
    }

    // If we aren't trying some level for the first time with 0.0 bestTime,
    // make sure there isn't a LOD that can be rendered faster and has a
    // higher level
    double level;
    if (bestTime != 0.0)
    {
      for (i = 0; i < this->NumberOfEntries; i++)
      {
        if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE && this->LODs[i].State == 1)
        {
          // Gather some information
          estimatedTime = this->GetLODIndexEstimatedRenderTime(i);
          level = this->GetLODIndexLevel(i);

          // Update the index and the level, but not the time. This is
          // so that we find the best level that can be rendered
          // faster than the LOD selected above.
          if (estimatedTime <= bestTime && level < bestLevel)
          {
            index = i;
            bestLevel = level;
          }
        }
      }
    }
  }
  else
  {
    index = 0;
    while (index < this->NumberOfEntries && this->LODs[index].ID != this->SelectedLODID)
    {
      index++;
    }
    if (index == this->NumberOfEntries)
    {
      svtkErrorMacro(<< "Could not render selected LOD ID: " << this->SelectedLODID);
      index = 0;
      while (index < this->NumberOfEntries && this->LODs[index].ID != SVTK_INDEX_NOT_IN_USE)
      {
        index++;
      }
    }
  }

  this->EstimatedRenderTime = 0.0;
  this->AllocatedRenderTime = t;
  if (index == -1)
  {
    return;
  }

  this->SelectedLODIndex = index;
  this->LODs[this->SelectedLODIndex].Prop3D->SetAllocatedRenderTime(t, vp);

  // Push the matrix down into the selected LOD
  svtkProp3D* p = this->LODs[this->SelectedLODIndex].Prop3D;
  // Getting our matrix here triggers a ComputeMatrix, if necessary,
  // which updates our MatrixMTime
  svtkMatrix4x4* mat = this->GetMatrix();
  if (p->GetUserTransformMatrixMTime() < this->MatrixMTime)
  {
    p->SetUserMatrix(mat);
  }
}

void svtkLODProp3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of LODs: " << this->NumberOfLODs << endl;

  os << indent << "Selected LOD ID: " << this->SelectedLODID << endl;

  os << indent << "AutomaticLODSelection: " << (this->AutomaticLODSelection ? "On\n" : "Off\n");

  os << indent
     << "AutomaticPickLODSelection: " << (this->AutomaticPickLODSelection ? "On\n" : "Off\n");

  os << indent << "SelectedPickLODID: " << this->SelectedPickLODID << endl;

  os << indent << "CurrentIndex: " << this->CurrentIndex << endl;
}

void svtkLODProp3D::GetActors(svtkPropCollection* ac)
{
#if 0
  // I don't get that 1999 code, why is it limiting the actors to the one
  // picked...

  svtkDebugMacro(<< "svtkLODProp3D::GetActors");
  int index;
  int lodID;

  lodID = this->GetPickLODID();
  index = this->ConvertIDToIndex(lodID);

  if (index == SVTK_INVALID_LOD_INDEX)
  {
    return;
  }

  if (! this->LODs[index].Prop3D->IsA("svtkVolume"))
  {
    ac->AddItem(this->LODs[index].Prop3D);
  }
#else
  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE && svtkActor::SafeDownCast(this->LODs[i].Prop3D))
    {
      ac->AddItem(this->LODs[i].Prop3D);
    }
  }
#endif
}

void svtkLODProp3D::GetVolumes(svtkPropCollection* ac)
{
  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE && svtkVolume::SafeDownCast(this->LODs[i].Prop3D))
    {
      ac->AddItem(this->LODs[i].Prop3D);
    }
  }
}

int svtkLODProp3D::GetAutomaticPickPropIndex()
{
  double bestTime = -1.0;
  int index = 0;
  double targetTime = 0;

  for (int i = 0; i < this->NumberOfEntries; i++)
  {
    if (this->LODs[i].ID != SVTK_INDEX_NOT_IN_USE)
    {
      // Gather some information
      double estimatedTime = this->GetLODIndexEstimatedRenderTime(i);

      // If we've never rendered this LOD and we have no info on it,
      // then try it out
      if (estimatedTime == 0.0)
      {
        index = i;
        break;
      }

      // If we do have at least a guess as to the render time, and
      // this seems like the best we have so far, pick it.
      // It is the best we have if
      //
      // 1) our estimated time is less than what we are looking for,
      //    but greater than any we have selected so far.
      //
      // 2) we have not selected anything else yet
      //    (regardless of what the estimated time is)
      //
      // 3) it is less than the time of the currently selected LOD
      //    if that LOD's time is greater than the time we are targeting.
      //
      if (estimatedTime > 0.0 &&
        ((estimatedTime > bestTime && estimatedTime < targetTime) || (bestTime == -1.0) ||
          (estimatedTime < bestTime && bestTime > targetTime)))
      {
        index = i;
        bestTime = estimatedTime;
      }
    }
  }
  return index;
}

void svtkLODProp3D::UpdateKeysForSelectedProp()
{
  this->LODs[this->SelectedLODIndex].Prop3D->SetPropertyKeys(this->GetPropertyKeys());
}

int svtkLODProp3D::GetPickLODID()
{
  svtkDebugMacro(<< "svtkLODProp3D::GetPickLODID");
  int lodID, index;
  if (this->AutomaticPickLODSelection)
  {
    if (this->SelectedLODIndex < 0 || this->SelectedLODIndex >= this->NumberOfEntries)
    {
      index = this->GetAutomaticPickPropIndex();
    }
    else
    {
      index = this->SelectedLODIndex;
    }
    lodID = this->LODs[index].ID;
  }
  else
  {
    lodID = this->SelectedPickLODID;
  }
  return lodID;
}

void svtkLODProp3D::SetSelectedPickLODID(int id)
{
  this->SelectedPickLODID = id;
  this->Modified();
}

void svtkLODProp3D::ShallowCopy(svtkProp* prop)
{
  svtkLODProp3D* a = svtkLODProp3D::SafeDownCast(prop);

  if (a)
  {
    this->SetAutomaticLODSelection(a->GetAutomaticLODSelection());
    this->SetAutomaticPickLODSelection(a->GetAutomaticPickLODSelection());
    this->SetSelectedLODID(a->GetSelectedLODID());
    this->NumberOfLODs = a->NumberOfLODs;
    // for(int i = 0; i < this->NumberOfLODs; i++) {}
  }

  // Now do superclass
  this->svtkProp3D::ShallowCopy(prop);
}
