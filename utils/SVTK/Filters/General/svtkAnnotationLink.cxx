/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAnnotationLink.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAnnotationLink.h"

#include "svtkAnnotationLayers.h"
#include "svtkCommand.h"
#include "svtkDataObjectCollection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkAnnotationLink);
// svtkCxxSetObjectMacro(svtkAnnotationLink, AnnotationLayers, svtkAnnotationLayers);

//---------------------------------------------------------------------------
// svtkAnnotationLink::Command
//----------------------------------------------------------------------------

class svtkAnnotationLink::Command : public svtkCommand
{
public:
  static Command* New() { return new Command(); }
  void Execute(svtkObject* caller, unsigned long eventId, void* callData) override
  {
    if (this->Target)
    {
      this->Target->ProcessEvents(caller, eventId, callData);
    }
  }
  void SetTarget(svtkAnnotationLink* t) { this->Target = t; }

private:
  Command() { this->Target = nullptr; }
  svtkAnnotationLink* Target;
};

//----------------------------------------------------------------------------
svtkAnnotationLink::svtkAnnotationLink()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(3);
  this->AnnotationLayers = svtkAnnotationLayers::New();
  this->DomainMaps = svtkDataObjectCollection::New();

  this->Observer = Command::New();
  this->Observer->SetTarget(this);
  this->AnnotationLayers->AddObserver(svtkCommand::ModifiedEvent, this->Observer);
}

//----------------------------------------------------------------------------
svtkAnnotationLink::~svtkAnnotationLink()
{
  this->Observer->Delete();

  if (this->AnnotationLayers)
  {
    this->AnnotationLayers->Delete();
  }
  if (this->DomainMaps)
  {
    this->DomainMaps->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::ProcessEvents(
  svtkObject* caller, unsigned long eventId, void* svtkNotUsed(callData))
{
  if (this->AnnotationLayers)
  {
    svtkAnnotationLayers* caller_annotations = svtkAnnotationLayers::SafeDownCast(caller);
    if (caller_annotations == this->AnnotationLayers && eventId == svtkCommand::ModifiedEvent)
    {
      this->InvokeEvent(svtkCommand::AnnotationChangedEvent, this->AnnotationLayers);
    }
  }
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::SetAnnotationLayers(svtkAnnotationLayers* layers)
{
  // This method is a cut and paste of svtkCxxSetObjectMacro
  // except that we listen for modified events from the annotations layers
  if (layers != this->AnnotationLayers)
  {
    svtkAnnotationLayers* tmp = this->AnnotationLayers;
    if (tmp)
    {
      tmp->RemoveObserver(this->Observer);
    }
    this->AnnotationLayers = layers;
    if (this->AnnotationLayers != nullptr)
    {
      this->AnnotationLayers->Register(this);
      this->AnnotationLayers->AddObserver(svtkCommand::ModifiedEvent, this->Observer);
    }
    if (tmp != nullptr)
    {
      tmp->UnRegister(this);
    }
    this->Modified();
    this->InvokeEvent(svtkCommand::AnnotationChangedEvent, this->AnnotationLayers);
  }
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::AddDomainMap(svtkTable* map)
{
  if (!this->DomainMaps->IsItemPresent(map))
  {
    this->DomainMaps->AddItem(map);
  }
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::RemoveDomainMap(svtkTable* map)
{
  this->DomainMaps->RemoveItem(map);
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::RemoveAllDomainMaps()
{
  if (this->DomainMaps->GetNumberOfItems() > 0)
  {
    this->DomainMaps->RemoveAllItems();
  }
}

//----------------------------------------------------------------------------
int svtkAnnotationLink::GetNumberOfDomainMaps()
{
  return this->DomainMaps->GetNumberOfItems();
}

//----------------------------------------------------------------------------
svtkTable* svtkAnnotationLink::GetDomainMap(int i)
{
  return svtkTable::SafeDownCast(this->DomainMaps->GetItem(i));
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::SetCurrentSelection(svtkSelection* sel)
{
  if (this->AnnotationLayers)
  {
    this->AnnotationLayers->SetCurrentSelection(sel);
  }
}

//----------------------------------------------------------------------------
svtkSelection* svtkAnnotationLink::GetCurrentSelection()
{
  if (this->AnnotationLayers)
  {
    return this->AnnotationLayers->GetCurrentSelection();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkAnnotationLink::RequestData(svtkInformation* svtkNotUsed(info),
  svtkInformationVector** inVector, svtkInformationVector* outVector)
{
  svtkInformation* inInfo = inVector[0]->GetInformationObject(0);
  svtkTable* inputMap = svtkTable::GetData(inVector[1]);
  svtkAnnotationLayers* input = nullptr;
  svtkSelection* inputSelection = nullptr;
  if (inInfo)
  {
    input = svtkAnnotationLayers::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    inputSelection = svtkSelection::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  svtkInformation* outInfo = outVector->GetInformationObject(0);
  svtkAnnotationLayers* output =
    svtkAnnotationLayers::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* mapInfo = outVector->GetInformationObject(1);
  svtkMultiBlockDataSet* maps =
    svtkMultiBlockDataSet::SafeDownCast(mapInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* selInfo = outVector->GetInformationObject(2);
  svtkSelection* sel = svtkSelection::SafeDownCast(selInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Give preference to input annotations
  if (input)
  {
    this->ShallowCopyToOutput(input, output, sel);
  }
  else if (this->AnnotationLayers)
  {
    this->ShallowCopyToOutput(this->AnnotationLayers, output, sel);
  }

  // If there is an input selection, set it on the annotation layers
  if (inputSelection)
  {
    sel->ShallowCopy(inputSelection);
    output->SetCurrentSelection(sel);
  }

  // If there are input domain maps, give preference to them
  if (inputMap)
  {
    svtkSmartPointer<svtkTable> outMap = svtkSmartPointer<svtkTable>::New();
    outMap->ShallowCopy(inputMap);
    maps->SetBlock(0, outMap);
  }
  else
  {
    unsigned int numMaps = static_cast<unsigned int>(this->DomainMaps->GetNumberOfItems());
    maps->SetNumberOfBlocks(numMaps);
    for (unsigned int i = 0; i < numMaps; ++i)
    {
      svtkSmartPointer<svtkTable> map = svtkSmartPointer<svtkTable>::New();
      map->ShallowCopy(this->DomainMaps->GetItem(i));
      maps->SetBlock(i, map);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::ShallowCopyToOutput(
  svtkAnnotationLayers* input, svtkAnnotationLayers* output, svtkSelection* sel)
{
  output->ShallowCopy(input);

  if (input->GetCurrentSelection())
  {
    sel->ShallowCopy(input->GetCurrentSelection());
  }
}

//----------------------------------------------------------------------------
int svtkAnnotationLink::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    // info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkAnnotationLink::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkAnnotationLayers");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
    return 1;
  }
  else if (port == 2)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkSelection");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkAnnotationLink::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();
  if (this->AnnotationLayers)
  {
    svtkMTimeType atime = this->AnnotationLayers->GetMTime();
    if (atime > mtime)
    {
      mtime = atime;
    }
  }

  if (this->DomainMaps)
  {
    svtkMTimeType dtime = this->DomainMaps->GetMTime();
    if (dtime > mtime)
    {
      mtime = dtime;
    }
  }

  return mtime;
}

//----------------------------------------------------------------------------
void svtkAnnotationLink::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnnotationLayers: ";
  if (this->AnnotationLayers)
  {
    os << "\n";
    this->AnnotationLayers->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "DomainMaps: ";
  if (this->DomainMaps)
  {
    os << "\n";
    this->DomainMaps->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
