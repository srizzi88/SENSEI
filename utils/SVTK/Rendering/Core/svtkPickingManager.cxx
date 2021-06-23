/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPickingManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*==============================================================================

  Library: MSSVTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// SVTK includes
#include "svtkPickingManager.h"
#include "svtkAbstractPicker.h"
#include "svtkAbstractPropPicker.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTimeStamp.h"

// STL includes
#include <algorithm>
#include <limits>
#include <map>
#include <vector>

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkPickingManager);

//------------------------------------------------------------------------------
class svtkPickingManager::svtkInternal
{
public:
  svtkInternal(svtkPickingManager* external);
  ~svtkInternal();

  // Callback used to update the current time
  // of the manager when an event occurs in the RenderWindowInteractor.
  // Time is used to know if the cached information is still valid or obsolete.
  static void UpdateTime(svtkObject* caller, unsigned long event, void* clientData, void* callData);

  // Select the best picker based on various criteria such as z-depth,
  // 2D overlay and/or distance to picked point.
  svtkAbstractPicker* SelectPicker();

  // Compute the selection. The current implementation use the distance
  // between the world coordinates of a pick to the camera's ones.
  svtkAbstractPicker* ComputePickerSelection(double X, double Y, double Z, svtkRenderer* renderer);

  // Check if a given Observator is associated with a given Picker
  bool IsObjectLinked(svtkAbstractPicker* picker, svtkObject* object);

  // Create a new list of associated observers
  void CreateDefaultCollection(svtkAbstractPicker* picker, svtkObject* object);

  // svtkCollection doesn't allow nullptr values. Instead we use a vector
  // containing svtkObject to allow using 0 as a valid value because it is
  // allowed the return a picker event if he is not associated to a specific
  // object.
  // This is related with the capacity when a picker associated with a given
  // object does not manage others object,
  // it will automatically be removed from the list as well.
  typedef std::vector<svtkObject*> CollectionType;

  // For code clearance and performances during the computation std::map is
  // used instead of a vector of pair. Nevertheless, it makes internally use of
  // svtkSmartPointer and this class does not overload the order operators;
  // therefore to following functor has to be implemented to keep the data
  // structure consistent.
  struct less_smartPtrPicker
  {
    bool operator()(const svtkSmartPointer<svtkAbstractPicker>& first,
      const svtkSmartPointer<svtkAbstractPicker>& second) const
    {
      return first < second;
    }
  };

  typedef std::map<svtkSmartPointer<svtkAbstractPicker>, CollectionType, less_smartPtrPicker>
    PickerObjectsType;

  typedef std::pair<svtkSmartPointer<svtkAbstractPicker>, CollectionType> PickerObjectsPairType;

  // Associate a given svtkObject to a particular picker.
  void LinkPickerObject(const PickerObjectsType::iterator& it, svtkObject* object);

  // Predicate comparing a svtkAbstractPicker*
  // and a svtkSmartPointer<svtkAbstractPicker> using the PickerObjectsType.
  // As we use a svtkSmartPointer, this predicate allows to compare the equality
  // of a pointer on a svtkAbstractPicker with the address contained in
  // a corresponding svtkSmartPointer.
  struct equal_smartPtrPicker
  {
    equal_smartPtrPicker(svtkAbstractPicker* picker)
      : Picker(picker)
    {
    }

    bool operator()(const PickerObjectsPairType& pickerObjs) const
    {
      return this->Picker == pickerObjs.first;
    }

    svtkAbstractPicker* Picker;
  };

  PickerObjectsType Pickers;           // Map the picker with the objects
  svtkTimeStamp CurrentInteractionTime; // Time of the last interaction event
  svtkTimeStamp LastPickingTime;        // Time of the last picking process
  svtkSmartPointer<svtkAbstractPicker> LastSelectedPicker;

  // Define callback to keep track of the CurrentTime and the LastPickingTime.
  // The timeStamp is use to avoid repeating the picking process if the
  // svtkWindowInteractor has not been modified, it is a huge optimization
  // avoiding each picker to relaunch the whole mechanisme to determine which
  // picker has been selected at a state of the rendering.
  svtkSmartPointer<svtkCallbackCommand> TimerCallback;

  svtkPickingManager* External;
};

//------------------------------------------------------------------------------
// svtkInternal methods

//------------------------------------------------------------------------------
svtkPickingManager::svtkInternal::svtkInternal(svtkPickingManager* external)
{
  this->External = external;

  this->TimerCallback = svtkSmartPointer<svtkCallbackCommand>::New();
  this->TimerCallback->SetClientData(this);
  this->TimerCallback->SetCallback(UpdateTime);
}

//------------------------------------------------------------------------------
svtkPickingManager::svtkInternal::~svtkInternal() = default;

//------------------------------------------------------------------------------
void svtkPickingManager::svtkInternal::CreateDefaultCollection(
  svtkAbstractPicker* picker, svtkObject* object)
{
  CollectionType objects;
  objects.push_back(object);

  this->Pickers.insert(PickerObjectsPairType(picker, objects));
}

//------------------------------------------------------------------------------
void svtkPickingManager::svtkInternal::LinkPickerObject(
  const PickerObjectsType::iterator& it, svtkObject* object)
{
  CollectionType::iterator itObj = std::find(it->second.begin(), it->second.end(), object);

  if (itObj != it->second.end() && object)
  {
    svtkDebugWithObjectMacro(this->External,
      "svtkPickingtManager::Internal::LinkPickerObject: "
        << "Current object already linked with the given picker.");

    return;
  }

  it->second.push_back(object);
}

//------------------------------------------------------------------------------
bool svtkPickingManager::svtkInternal::IsObjectLinked(svtkAbstractPicker* picker, svtkObject* obj)
{
  if (!picker || !obj)
  {
    return false;
  }

  PickerObjectsType::iterator itPick =
    std::find_if(this->Pickers.begin(), this->Pickers.end(), equal_smartPtrPicker(picker));
  if (itPick == this->Pickers.end())
  {
    return false;
  }

  CollectionType::iterator itObj = std::find(itPick->second.begin(), itPick->second.end(), obj);
  return (itObj != itPick->second.end());
}

//------------------------------------------------------------------------------
svtkAbstractPicker* svtkPickingManager::svtkInternal::SelectPicker()
{
  if (!this->External->Interactor)
  {
    return nullptr;
  }
  else if (this->External->GetOptimizeOnInteractorEvents() &&
    this->CurrentInteractionTime.GetMTime() == this->LastPickingTime)
  {
    return this->LastSelectedPicker;
  }

  // Get the event position
  double X = this->External->Interactor->GetEventPosition()[0];
  double Y = this->External->Interactor->GetEventPosition()[1];

  // Get the poked renderer
  svtkRenderer* renderer =
    this->External->Interactor->FindPokedRenderer(static_cast<int>(X), static_cast<int>(Y));
  svtkAbstractPicker* selectedPicker = this->ComputePickerSelection(X, Y, 0., renderer);

  // Keep track of the last picker chosen & last picking time.
  this->LastSelectedPicker = selectedPicker;
  this->LastPickingTime = this->CurrentInteractionTime;

  return selectedPicker;
}

//------------------------------------------------------------------------------
svtkAbstractPicker* svtkPickingManager::svtkInternal::ComputePickerSelection(
  double X, double Y, double Z, svtkRenderer* renderer)
{
  svtkAbstractPicker* closestPicker = nullptr;
  if (!renderer)
  {
    return closestPicker;
  }

  double* camPos = renderer->GetActiveCamera()->GetPosition();
  double smallestDistance2 = std::numeric_limits<double>::max();

  for (PickerObjectsType::iterator it = this->Pickers.begin(); it != this->Pickers.end(); ++it)
  {
    int pickResult = it->first->Pick(X, Y, Z, renderer);
    double* pPos = it->first->GetPickPosition();

    if (pickResult > 0) // Keep closest object picked.
    {
      double distance2 = svtkMath::Distance2BetweenPoints(camPos, pPos);

      if (smallestDistance2 > distance2)
      {
        smallestDistance2 = distance2;
        closestPicker = it->first;
      }
    }
  }

  return closestPicker;
}

//------------------------------------------------------------------------------
void svtkPickingManager::svtkInternal::UpdateTime(svtkObject* svtkNotUsed(caller),
  unsigned long svtkNotUsed(event), void* clientData, void* svtkNotUsed(calldata))
{
  svtkPickingManager::svtkInternal* self =
    reinterpret_cast<svtkPickingManager::svtkInternal*>(clientData);
  if (!self)
  {
    return;
  }

  self->CurrentInteractionTime.Modified();
}

//------------------------------------------------------------------------------
// svtkPickingManager methods

//------------------------------------------------------------------------------
svtkPickingManager::svtkPickingManager()
  : Interactor(nullptr)
  , Enabled(false)
  , OptimizeOnInteractorEvents(true)
  , Internal(nullptr)
{
  this->Internal = new svtkInternal(this);
}

//------------------------------------------------------------------------------
svtkPickingManager::~svtkPickingManager()
{
  this->SetInteractor(nullptr);
  delete this->Internal;
}

//------------------------------------------------------------------------------
void svtkPickingManager::SetInteractor(svtkRenderWindowInteractor* rwi)
{
  if (rwi == this->Interactor)
  {
    return;
  }
  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->Internal->TimerCallback);
  }

  this->Interactor = rwi;
  if (this->Interactor)
  {
    this->Interactor->AddObserver(svtkCommand::ModifiedEvent, this->Internal->TimerCallback);
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void svtkPickingManager::SetOptimizeOnInteractorEvents(bool optimize)
{
  if (this->OptimizeOnInteractorEvents == optimize)
  {
    return;
  }

  this->OptimizeOnInteractorEvents = optimize;
  this->Modified();
}

//------------------------------------------------------------------------------
void svtkPickingManager::AddPicker(svtkAbstractPicker* picker, svtkObject* object)
{
  if (!picker)
  {
    return;
  }

  // Linke the object if the picker is already registered
  svtkPickingManager::svtkInternal::PickerObjectsType::iterator it =
    std::find_if(this->Internal->Pickers.begin(), this->Internal->Pickers.end(),
      svtkPickingManager::svtkInternal::equal_smartPtrPicker(picker));

  if (it != this->Internal->Pickers.end())
  {
    svtkDebugMacro("svtkPickingtManager::AddPicker: "
      << "Picker already in the manager, the object will be linked");

    this->Internal->LinkPickerObject(it, object);
    return;
  }

  // The picker does not exists in the manager yet.
  // Create the list of associated objects
  this->Internal->CreateDefaultCollection(picker, object);
}

//------------------------------------------------------------------------------
void svtkPickingManager::RemovePicker(svtkAbstractPicker* picker, svtkObject* object)
{
  svtkPickingManager::svtkInternal::PickerObjectsType::iterator it =
    std::find_if(this->Internal->Pickers.begin(), this->Internal->Pickers.end(),
      svtkPickingManager::svtkInternal::equal_smartPtrPicker(picker));

  // The Picker does not exist
  if (it == this->Internal->Pickers.end())
  {
    return;
  }

  svtkPickingManager::svtkInternal::CollectionType::iterator itObj =
    std::find(it->second.begin(), it->second.end(), object);

  // The object is not associated with the given picker.
  if (itObj == it->second.end())
  {
    return;
  }

  it->second.erase(itObj);

  // Delete the picker when it is not associated with any object anymore.
  if (it->second.empty())
  {
    this->Internal->Pickers.erase(it);
  }
}

//------------------------------------------------------------------------------
void svtkPickingManager::RemoveObject(svtkObject* object)
{
  svtkPickingManager::svtkInternal::PickerObjectsType::iterator it = this->Internal->Pickers.begin();

  for (; it != this->Internal->Pickers.end();)
  {
    svtkPickingManager::svtkInternal::CollectionType::iterator itObj =
      std::find(it->second.begin(), it->second.end(), object);

    if (itObj != it->second.end())
    {
      it->second.erase(itObj);

      if (it->second.empty())
      {
        svtkPickingManager::svtkInternal::PickerObjectsType::iterator toRemove = it;
        ++it;
        this->Internal->Pickers.erase(toRemove);
        continue;
      }
    }

    ++it;
  }
}

//------------------------------------------------------------------------------
bool svtkPickingManager::Pick(svtkAbstractPicker* picker, svtkObject* obj)
{
  if (!this->Internal->IsObjectLinked(picker, obj))
  {
    return false;
  }

  return (this->Pick(picker));
}

//------------------------------------------------------------------------------
bool svtkPickingManager::Pick(svtkObject* obj)
{
  svtkAbstractPicker* picker = this->Internal->SelectPicker();
  if (!picker)
  {
    return false;
  }
  // If the object is not contained in the list of the associated active pickers
  // return false
  return (this->Internal->IsObjectLinked(picker, obj));
}

//------------------------------------------------------------------------------
bool svtkPickingManager::Pick(svtkAbstractPicker* picker)
{
  return (picker == this->Internal->SelectPicker());
}

//------------------------------------------------------------------------------
svtkAssemblyPath* svtkPickingManager::GetAssemblyPath(double X, double Y, double Z,
  svtkAbstractPropPicker* picker, svtkRenderer* renderer, svtkObject* obj)
{
  if (this->Enabled)
  {
    // Return 0 when the Picker is not selected
    if (!this->Pick(picker, obj))
    {
      return nullptr;
    }
  }
  else
  {
    picker->Pick(X, Y, Z, renderer);
  }

  return picker->GetPath();
}

//------------------------------------------------------------------------------
int svtkPickingManager::GetNumberOfPickers()
{
  return static_cast<int>(this->Internal->Pickers.size());
}

//------------------------------------------------------------------------------
int svtkPickingManager::GetNumberOfObjectsLinked(svtkAbstractPicker* picker)
{
  if (!picker)
  {
    return 0;
  }

  svtkPickingManager::svtkInternal::PickerObjectsType::iterator it =
    std::find_if(this->Internal->Pickers.begin(), this->Internal->Pickers.end(),
      svtkPickingManager::svtkInternal::equal_smartPtrPicker(picker));

  if (it == this->Internal->Pickers.end())
  {
    return 0;
  }

  return static_cast<int>(it->second.size());
}

//------------------------------------------------------------------------------
void svtkPickingManager::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderWindowInteractor: " << this->Interactor << "\n";
  os << indent << "NumberOfPickers: " << this->Internal->Pickers.size() << "\n";

  svtkPickingManager::svtkInternal::PickerObjectsType::iterator it = this->Internal->Pickers.begin();

  for (; it != this->Internal->Pickers.end(); ++it)
  {
    os << indent << indent << "Picker: " << it->first << "\n";
    os << indent << indent << "NumberOfObjectsLinked: " << it->second.size() << "\n";
  }
}
