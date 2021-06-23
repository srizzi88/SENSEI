/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPickingManager.cxx

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

// .NAME Test of PickingManager.
// .SECTION Description
// Tests PickingManager internal data structure.

// SVTK includes
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPicker.h"
#include "svtkPickingManager.h"
#include "svtkSmartPointer.h"

#define SVTK_VERIFY(test, errorStr) this->SVTKVerify((test), (errorStr), __LINE__)

class PickingManagerTest
{
public:
  bool TestProperties();
  bool TestAddPickers();
  bool TestRemovePickers();
  bool TestRemoveObjects();
  bool TestObjectOwnership();

  bool SVTKVerify(bool test, const char* errorStr, int line);
  void PrintErrorMessage(int line, const char* errorStr);

protected:
  std::pair<svtkSmartPointer<svtkPicker>, svtkSmartPointer<svtkObject> > AddPickerObject(
    int pickerType, int objectType);

  bool AddPicker(int pickerType, int objectType, int numberOfPickers, int numberOfObjectsLinked);

  bool AddPickerTwice(int pickerType0, int objectType0, int pickerType1, int objectType1,
    bool samePicker, int numberOfPickers, int numberOfObjectsLinked0, int numberOfObjectsLinked1);

  bool RemovePicker(int pickerType, int numberOfPickers);
  bool RemoveOneOfPickers(int pickerType0, int objectType0, int pickerType1, int objectType1,
    bool samePicker, int numberOfPickers, int numberOfObjectsLinked0, int numberOfObjectsLinked1);

  bool RemoveObject(
    int pickerType, int objectType, int numberOfPickers, int numberOfObjectsLinked1);

  bool CheckState(int numberOfPickers, svtkPicker* picker = nullptr, int NumberOfObjectsLinked = 0);

private:
  svtkSmartPointer<svtkPickingManager> PickingManager;
};

//------------------------------------------------------------------------------
// Test picking manager client that removes itself from the picking manager
// in its destructor. This mimics the behavior of the SVTK widget framework.
class PickingManagerClient : public svtkObject
{
public:
  static PickingManagerClient* New();
  svtkTypeMacro(PickingManagerClient, svtkObject);

  void SetPickingManager(svtkPickingManager* pm);
  void RegisterPicker();
  svtkPicker* GetPicker();

protected:
  PickingManagerClient();
  ~PickingManagerClient() override;

private:
  svtkPickingManager* PickingManager;
  svtkPicker* Picker;

  PickingManagerClient(const PickingManagerClient&) = delete;
  void operator=(const PickingManagerClient&) = delete;
};

svtkStandardNewMacro(PickingManagerClient);

//------------------------------------------------------------------------------
PickingManagerClient::PickingManagerClient()
{
  this->Picker = svtkPicker::New();
}

//------------------------------------------------------------------------------
PickingManagerClient::~PickingManagerClient()
{
  this->Picker->Delete();

  if (this->PickingManager)
  {
    this->PickingManager->RemoveObject(this);
  }
}

//------------------------------------------------------------------------------
void PickingManagerClient::SetPickingManager(svtkPickingManager* pm)
{
  this->PickingManager = pm;
}

//------------------------------------------------------------------------------
void PickingManagerClient::RegisterPicker()
{
  if (!this->PickingManager)
  {
    return;
  }

  this->PickingManager->AddPicker(this->Picker, this);
}

//------------------------------------------------------------------------------
svtkPicker* PickingManagerClient::GetPicker()
{
  return this->Picker;
}

//------------------------------------------------------------------------------
int TestPickingManager(int, char*[])
{
  PickingManagerTest pickingManagerTest;

  bool res = true;

  res = res && pickingManagerTest.TestProperties();
  res = res && pickingManagerTest.TestAddPickers();
  res = res && pickingManagerTest.TestRemovePickers();
  res = res && pickingManagerTest.TestRemoveObjects();
  res = res && pickingManagerTest.TestObjectOwnership();

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestProperties()
{
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();

  bool res = true;

  // Default
  res =
    SVTK_VERIFY(this->PickingManager->GetEnabled() == 0, "Error manager not disabled by default") &&
    res;
  res = SVTK_VERIFY(this->PickingManager->GetOptimizeOnInteractorEvents() == 1,
          "Error optimizationOnInteractor not enabled by default:") &&
    res;
  res = SVTK_VERIFY(this->PickingManager->GetInteractor() == nullptr,
          "Error interactor not null by default:") &&
    res;
  res = SVTK_VERIFY(this->PickingManager->GetNumberOfPickers() == 0,
          "Error numberOfPickers not nul by default:") &&
    res;
  res = SVTK_VERIFY(this->PickingManager->GetNumberOfObjectsLinked(nullptr) == 0,
          "Error NumberOfObjectsLinked not nul with null picker:") &&
    res;

  // Settings properties
  this->PickingManager->EnabledOn();
  res = SVTK_VERIFY(this->PickingManager->GetEnabled() == 1, "Error manager not does not enable:") &&
    res;
  this->PickingManager->SetOptimizeOnInteractorEvents(false);
  res = SVTK_VERIFY(this->PickingManager->GetOptimizeOnInteractorEvents() == 0,
          "Error OptimizeOnInteractorEvents does not get disabled:") &&
    res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestAddPickers()
{
  bool res = true;

  // Simple Add
  res = SVTK_VERIFY(this->AddPicker(0, 0, 0, 0), "Error adding a null picker:") && res;
  res =
    SVTK_VERIFY(this->AddPicker(0, 1, 0, 0), "Error adding a null picker with an object:") && res;
  res = SVTK_VERIFY(this->AddPicker(1, 0, 1, 1), "Error adding a picker with a null object:") && res;
  res = SVTK_VERIFY(this->AddPicker(1, 1, 1, 1), "Error adding a picker with an object:") && res;

  // Twice Add
  res = SVTK_VERIFY(this->AddPickerTwice(1, 0, 1, 0, false, 2, 1, 1),
          "Error adding two pickers with null objects:") &&
    res;
  res = SVTK_VERIFY(this->AddPickerTwice(1, 0, 1, 0, true, 1, 2, 2),
          "Error adding same picker with null objects:") &&
    res;
  res = SVTK_VERIFY(this->AddPickerTwice(1, 1, 1, 1, false, 2, 1, 1),
          "Error adding pickers with valid objects:") &&
    res;
  res = SVTK_VERIFY(this->AddPickerTwice(1, 1, 1, 1, true, 1, 2, 2),
          "Error adding same picker with valid objects:") &&
    res;

  // Particular case: same picker with same valid object
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkNew<svtkPicker> picker;
  svtkNew<svtkObject> object;
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->AddPicker(picker, object);

  res =
    SVTK_VERIFY(this->CheckState(1, picker, 1), "Error adding same picker with same object:") && res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestRemovePickers()
{
  bool res = true;

  // Remove Picker following a simple add
  res = SVTK_VERIFY(this->RemovePicker(0, 0), "Error removing null picker:") && res;
  res = SVTK_VERIFY(this->RemovePicker(1, 0), "Error removing existing picker:") && res;

  // Remove Picker following a multiples add
  res = SVTK_VERIFY(this->RemoveOneOfPickers(1, 0, 1, 0, false, 1, 0, 1),
          "Error removing a picker with null object:") &&
    res;
  res = SVTK_VERIFY(this->RemoveOneOfPickers(1, 0, 1, 0, true, 1, 1, 1),
          "Error removing a picker with null objects:") &&
    res;
  res = SVTK_VERIFY(this->RemoveOneOfPickers(1, 1, 1, 1, true, 1, 1, 1),
          "Error adding pickers with valid objects:") &&
    res;

  // Particular case same picker with same valid object
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkNew<svtkPicker> picker;
  svtkNew<svtkObject> object;
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->RemovePicker(picker, object);

  res =
    SVTK_VERIFY(this->CheckState(0, picker, 0), "Error removing a picker with same object:") && res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestRemoveObjects()
{
  bool res = true;

  // Remove Object following a simple add
  res =
    SVTK_VERIFY(this->RemoveObject(0, 0, 0, 0), "Error removing null object without picker:") && res;
  res =
    SVTK_VERIFY(this->RemoveObject(1, 0, 0, 0), "Error removing null object with a picker:") && res;
  res = SVTK_VERIFY(this->RemoveObject(0, 1, 0, 0), "Error removing object without picker:") && res;
  res = SVTK_VERIFY(this->RemoveObject(1, 1, 0, 0), "Error removing object with a picker:") && res;

  // Particular cases same picker with same valid object
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkNew<svtkPicker> picker;
  svtkNew<svtkObject> object;
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->RemoveObject(object);

  res =
    SVTK_VERIFY(this->CheckState(0, picker, 0), "Error removing an object with same picker:") && res;

  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkNew<svtkObject> object2;
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->AddPicker(picker, object2);
  this->PickingManager->RemoveObject(object);

  res = SVTK_VERIFY(
          this->CheckState(1, picker, 1), "Error removing one of the objects with same picker:") &&
    res;

  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkNew<svtkPicker> picker2;
  this->PickingManager->AddPicker(picker, object);
  this->PickingManager->AddPicker(picker2, object);
  this->PickingManager->RemoveObject(object);

  res =
    SVTK_VERIFY(this->CheckState(0, picker, 0), "Error removing object with different pickers:") &&
    res;

  return res;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::TestObjectOwnership()
{
  bool res = true;

  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkSmartPointer<PickingManagerClient> client = svtkSmartPointer<PickingManagerClient>::New();
  client->SetPickingManager(this->PickingManager);
  client->RegisterPicker();

  res = SVTK_VERIFY(
          this->CheckState(1, client->GetPicker(), 1), "Error after client registers picker:") &&
    res;

  client = nullptr;

  res =
    SVTK_VERIFY(this->CheckState(0, nullptr, 0), "Error after setting client object to nullptr:") &&
    res;

  return res;
}

//------------------------------------------------------------------------------
std::pair<svtkSmartPointer<svtkPicker>, svtkSmartPointer<svtkObject> >
PickingManagerTest::AddPickerObject(int pickerType, int objectType)
{
  svtkSmartPointer<svtkPicker> picker =
    (pickerType == 0) ? nullptr : svtkSmartPointer<svtkPicker>::New();
  svtkSmartPointer<svtkObject> object =
    (objectType == 0) ? nullptr : svtkSmartPointer<svtkObject>::New();

  this->PickingManager->AddPicker(picker, object);

  return std::pair<svtkSmartPointer<svtkPicker>, svtkSmartPointer<svtkObject> >(picker, object);
}

//------------------------------------------------------------------------------
bool PickingManagerTest::AddPicker(
  int pickerType, int objectType, int numberOfPickers, int numberOfObjectsLinked)
{
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();

  svtkSmartPointer<svtkPicker> picker = this->AddPickerObject(pickerType, objectType).first;

  return this->CheckState(numberOfPickers, picker, numberOfObjectsLinked);
}

//------------------------------------------------------------------------------
bool PickingManagerTest::AddPickerTwice(int pickerType0, int objectType0, int pickerType1,
  int objectType1, bool samePicker, int numberOfPickers, int numberOfObjectsLinked0,
  int numberOfObjectsLinked1)
{
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();

  svtkSmartPointer<svtkPicker> picker0 = this->AddPickerObject(pickerType0, objectType0).first;

  svtkSmartPointer<svtkPicker> picker1 =
    (samePicker) ? picker0 : this->AddPickerObject(pickerType1, objectType1).first;

  if (samePicker)
  {
    this->PickingManager->AddPicker(picker1);
  }

  return (this->CheckState(numberOfPickers, picker0, numberOfObjectsLinked0) &&
    this->CheckState(numberOfPickers, picker1, numberOfObjectsLinked1));
}

//------------------------------------------------------------------------------
bool PickingManagerTest::RemovePicker(int pickerType, int numberOfPickers)
{
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();
  svtkSmartPointer<svtkPicker> picker = this->AddPickerObject(pickerType, 0).first;

  this->PickingManager->RemovePicker(picker);

  return this->CheckState(numberOfPickers, nullptr, 0);
}

//------------------------------------------------------------------------------
bool PickingManagerTest::RemoveOneOfPickers(int pickerType0, int objectType0, int pickerType1,
  int objectType1, bool samePicker, int numberOfPickers, int numberOfObjectsLinked0,
  int numberOfObjectsLinked1)
{
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();

  svtkSmartPointer<svtkPicker> picker0 = this->AddPickerObject(pickerType0, objectType0).first;

  svtkSmartPointer<svtkPicker> picker1 =
    (samePicker) ? picker0 : this->AddPickerObject(pickerType1, objectType1).first;

  if (samePicker)
  {
    this->PickingManager->AddPicker(picker1);
  }

  this->PickingManager->RemovePicker(picker0);

  return (this->CheckState(numberOfPickers, picker0, numberOfObjectsLinked0) &&
    this->CheckState(numberOfPickers, picker1, numberOfObjectsLinked1));
}

//------------------------------------------------------------------------------
bool PickingManagerTest::RemoveObject(
  int pickerType, int objectType, int numberOfPickers, int numberOfObjectsLinked1)
{
  this->PickingManager = svtkSmartPointer<svtkPickingManager>::New();

  std::pair<svtkSmartPointer<svtkPicker>, svtkSmartPointer<svtkObject> > pickerObject =
    this->AddPickerObject(pickerType, objectType);

  this->PickingManager->RemoveObject(pickerObject.second);

  return this->CheckState(numberOfPickers, pickerObject.first, numberOfObjectsLinked1);
}

//------------------------------------------------------------------------------
void PickingManagerTest::PrintErrorMessage(int line, const char* errorStr)
{
  std::cout << line << ": " << errorStr << "\n";

  if (PickingManager)
  {
    PickingManager->Print(std::cout);
  }
}

//------------------------------------------------------------------------------
bool PickingManagerTest::SVTKVerify(bool test, const char* errorStr, int line = -1)
{
  if (!test)
  {
    this->PrintErrorMessage(line, errorStr);
  }

  return test;
}

//------------------------------------------------------------------------------
bool PickingManagerTest::CheckState(
  int numberOfPickers, svtkPicker* picker, int numberOfObjectsLinked)
{
  return (this->PickingManager->GetNumberOfPickers() == numberOfPickers &&
    this->PickingManager->GetNumberOfObjectsLinked(picker) == numberOfObjectsLinked);
}
