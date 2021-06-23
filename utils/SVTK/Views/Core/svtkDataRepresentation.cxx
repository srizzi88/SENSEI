/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkDataRepresentation.h"

#include "svtkAlgorithmOutput.h"
#include "svtkAnnotationLayers.h"
#include "svtkAnnotationLink.h"
#include "svtkCommand.h"
#include "svtkConvertSelectionDomain.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTrivialProducer.h"

#include <map>

//---------------------------------------------------------------------------
// svtkDataRepresentation::Internals
//---------------------------------------------------------------------------

class svtkDataRepresentation::Internals
{
public:
  // This is a cache of shallow copies of inputs provided for convenience.
  // It is a map from (port index, connection index) to (original input data port, shallow copy
  // port). NOTE: The original input data port pointer is not reference counted, so it should not be
  // assumed to be a valid pointer. It is only used for pointer comparison.
  std::map<std::pair<int, int>,
    std::pair<svtkAlgorithmOutput*, svtkSmartPointer<svtkTrivialProducer> > >
    InputInternal;

  // This is a cache of svtkConvertSelectionDomain filters provided for convenience.
  // It is a map from (port index, connection index) to convert selection domain filter.
  std::map<std::pair<int, int>, svtkSmartPointer<svtkConvertSelectionDomain> > ConvertDomainInternal;
};

//---------------------------------------------------------------------------
// svtkDataRepresentation::Command
//----------------------------------------------------------------------------

class svtkDataRepresentation::Command : public svtkCommand
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
  void SetTarget(svtkDataRepresentation* t) { this->Target = t; }

private:
  Command() { this->Target = nullptr; }
  svtkDataRepresentation* Target;
};

//----------------------------------------------------------------------------
// svtkDataRepresentation
//----------------------------------------------------------------------------

svtkStandardNewMacro(svtkDataRepresentation);
svtkCxxSetObjectMacro(svtkDataRepresentation, AnnotationLinkInternal, svtkAnnotationLink);
svtkCxxSetObjectMacro(svtkDataRepresentation, SelectionArrayNames, svtkStringArray);

//----------------------------------------------------------------------------
svtkTrivialProducer* svtkDataRepresentation::GetInternalInput(int port, int conn)
{
  return this->Implementation->InputInternal[std::pair<int, int>(port, conn)].second;
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::SetInternalInput(int port, int conn, svtkTrivialProducer* producer)
{
  this->Implementation->InputInternal[std::pair<int, int>(port, conn)] =
    std::pair<svtkAlgorithmOutput*, svtkSmartPointer<svtkTrivialProducer> >(
      this->GetInputConnection(port, conn), producer);
}

//----------------------------------------------------------------------------
svtkDataRepresentation::svtkDataRepresentation()
{
  this->Implementation = new svtkDataRepresentation::Internals();
  // Listen to event indicating that the algorithm is done executing.
  // We may need to clear the data object cache after execution.
  this->Observer = Command::New();
  this->AddObserver(svtkCommand::EndEvent, this->Observer);

  this->Selectable = true;
  this->SelectionArrayNames = svtkStringArray::New();
  this->SelectionType = svtkSelectionNode::INDICES;
  this->AnnotationLinkInternal = svtkAnnotationLink::New();
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
svtkDataRepresentation::~svtkDataRepresentation()
{
  delete this->Implementation;
  this->Observer->Delete();
  this->SetSelectionArrayNames(nullptr);
  this->SetAnnotationLinkInternal(nullptr);
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::SetAnnotationLink(svtkAnnotationLink* link)
{
  this->SetAnnotationLinkInternal(link);
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::ProcessEvents(
  svtkObject* caller, unsigned long eventId, void* svtkNotUsed(callData))
{
  // After the algorithm executes, if the release data flag is on,
  // clear the input shallow copy cache.
  if (caller == this && eventId == svtkCommand::EndEvent)
  {
    // Release input data if requested.
    for (int i = 0; i < this->GetNumberOfInputPorts(); ++i)
    {
      for (int j = 0; j < this->GetNumberOfInputConnections(i); ++j)
      {
        svtkInformation* inInfo = this->GetExecutive()->GetInputInformation(i, j);
        svtkDataObject* dataObject = inInfo->Get(svtkDataObject::DATA_OBJECT());
        if (dataObject &&
          (dataObject->GetGlobalReleaseDataFlag() ||
            inInfo->Get(svtkDemandDrivenPipeline::RELEASE_DATA())))
        {
          std::pair<int, int> p(i, j);
          this->Implementation->InputInternal.erase(p);
          this->Implementation->ConvertDomainInternal.erase(p);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkDataRepresentation::GetInternalOutputPort(int port, int conn)
{
  if (port >= this->GetNumberOfInputPorts() || conn >= this->GetNumberOfInputConnections(port))
  {
    svtkErrorMacro(
      "Port " << port << ", connection " << conn << " is not defined on this representation.");
    return nullptr;
  }

  // The cached shallow copy is out of date when the input data object
  // changed, or the shallow copy modified time is less than the
  // input modified time.
  std::pair<int, int> p(port, conn);
  svtkAlgorithmOutput* input = this->GetInputConnection(port, conn);
  svtkDataObject* inputDObj = this->GetInputDataObject(port, conn);
  if (this->Implementation->InputInternal.find(p) == this->Implementation->InputInternal.end() ||
    this->Implementation->InputInternal[p].first != input ||
    this->Implementation->InputInternal[p].second->GetMTime() < inputDObj->GetMTime())
  {
    this->Implementation->InputInternal[p].first = input;
    svtkDataObject* copy = inputDObj->NewInstance();
    copy->ShallowCopy(inputDObj);
    svtkTrivialProducer* tp = svtkTrivialProducer::New();
    tp->SetOutput(copy);
    copy->Delete();
    this->Implementation->InputInternal[p].second = tp;
    tp->Delete();
  }

  return this->Implementation->InputInternal[p].second->GetOutputPort();
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkDataRepresentation::GetInternalAnnotationOutputPort(int port, int conn)
{
  if (port >= this->GetNumberOfInputPorts() || conn >= this->GetNumberOfInputConnections(port))
  {
    svtkErrorMacro(
      "Port " << port << ", connection " << conn << " is not defined on this representation.");
    return nullptr;
  }

  // Create a new filter in the cache if necessary.
  std::pair<int, int> p(port, conn);
  if (this->Implementation->ConvertDomainInternal.find(p) ==
    this->Implementation->ConvertDomainInternal.end())
  {
    this->Implementation->ConvertDomainInternal[p] =
      svtkSmartPointer<svtkConvertSelectionDomain>::New();
  }

  // Set up the inputs to the cached filter.
  svtkConvertSelectionDomain* domain = this->Implementation->ConvertDomainInternal[p];
  domain->SetInputConnection(0, this->GetAnnotationLink()->GetOutputPort(0));
  domain->SetInputConnection(1, this->GetAnnotationLink()->GetOutputPort(1));
  domain->SetInputConnection(2, this->GetInternalOutputPort(port, conn));

  // Output port 0 of the convert domain filter is the linked
  // annotation(s) (the svtkAnnotationLayers object).
  return domain->GetOutputPort();
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkDataRepresentation::GetInternalSelectionOutputPort(int port, int conn)
{
  // First make sure the convert domain filter is up to date.
  if (!this->GetInternalAnnotationOutputPort(port, conn))
  {
    return nullptr;
  }

  // Output port 1 of the convert domain filter is the current selection
  // that was contained in the linked annotation.
  std::pair<int, int> p(port, conn);
  if (this->Implementation->ConvertDomainInternal.find(p) !=
    this->Implementation->ConvertDomainInternal.end())
  {
    return this->Implementation->ConvertDomainInternal[p]->GetOutputPort(1);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::Select(svtkView* view, svtkSelection* selection, bool extend)
{
  if (this->Selectable)
  {
    svtkSelection* converted = this->ConvertSelection(view, selection);
    if (converted)
    {
      this->UpdateSelection(converted, extend);
      if (converted != selection)
      {
        converted->Delete();
      }
    }
  }
}

//----------------------------------------------------------------------------
svtkSelection* svtkDataRepresentation::ConvertSelection(
  svtkView* svtkNotUsed(view), svtkSelection* selection)
{
  return selection;
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::UpdateSelection(svtkSelection* selection, bool extend)
{
  if (extend)
  {
    selection->Union(this->AnnotationLinkInternal->GetCurrentSelection());
  }
  this->AnnotationLinkInternal->SetCurrentSelection(selection);
  this->InvokeEvent(svtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(selection));
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::Annotate(svtkView* view, svtkAnnotationLayers* annotations, bool extend)
{
  svtkAnnotationLayers* converted = this->ConvertAnnotations(view, annotations);
  if (converted)
  {
    this->UpdateAnnotations(converted, extend);
    if (converted != annotations)
    {
      converted->Delete();
    }
  }
}

//----------------------------------------------------------------------------
svtkAnnotationLayers* svtkDataRepresentation::ConvertAnnotations(
  svtkView* svtkNotUsed(view), svtkAnnotationLayers* annotations)
{
  return annotations;
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::UpdateAnnotations(svtkAnnotationLayers* annotations, bool extend)
{
  if (extend)
  {
    // Append the annotations to the existing set of annotations on the link
    svtkAnnotationLayers* currentAnnotations = this->AnnotationLinkInternal->GetAnnotationLayers();
    for (unsigned int i = 0; i < annotations->GetNumberOfAnnotations(); ++i)
    {
      currentAnnotations->AddAnnotation(annotations->GetAnnotation(i));
    }
    this->InvokeEvent(
      svtkCommand::AnnotationChangedEvent, reinterpret_cast<void*>(currentAnnotations));
  }
  else
  {
    this->AnnotationLinkInternal->SetAnnotationLayers(annotations);
    this->InvokeEvent(svtkCommand::AnnotationChangedEvent, reinterpret_cast<void*>(annotations));
  }
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::SetSelectionArrayName(const char* name)
{
  if (!this->SelectionArrayNames)
  {
    this->SelectionArrayNames = svtkStringArray::New();
  }
  this->SelectionArrayNames->Initialize();
  this->SelectionArrayNames->InsertNextValue(name);
}

//----------------------------------------------------------------------------
const char* svtkDataRepresentation::GetSelectionArrayName()
{
  if (this->SelectionArrayNames && this->SelectionArrayNames->GetNumberOfTuples() > 0)
  {
    return this->SelectionArrayNames->GetValue(0);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkDataRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnnotationLink: " << (this->AnnotationLinkInternal ? "" : "(null)") << endl;
  if (this->AnnotationLinkInternal)
  {
    this->AnnotationLinkInternal->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Selectable: " << this->Selectable << endl;
  os << indent << "SelectionType: " << this->SelectionType << endl;
  os << indent << "SelectionArrayNames: " << (this->SelectionArrayNames ? "" : "(null)") << endl;
  if (this->SelectionArrayNames)
  {
    this->SelectionArrayNames->PrintSelf(os, indent.GetNextIndent());
  }
}
