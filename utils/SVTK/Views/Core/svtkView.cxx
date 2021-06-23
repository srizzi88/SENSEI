/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkView.cxx

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

#include "svtkView.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCollection.h"
#include "svtkCommand.h"
#include "svtkDataObject.h"
#include "svtkDataRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTrivialProducer.h"
#include "svtkViewTheme.h"

#include <map>
#include <string>
#include <vector>

//----------------------------------------------------------------------------
class svtkView::Command : public svtkCommand
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
  void SetTarget(svtkView* t) { this->Target = t; }

private:
  Command() { this->Target = nullptr; }
  svtkView* Target;
};

//----------------------------------------------------------------------------
class svtkView::svtkInternal
{
public:
  std::map<svtkObject*, std::string> RegisteredProgress;
};

//----------------------------------------------------------------------------
class svtkView::svtkImplementation
{
public:
  std::vector<svtkSmartPointer<svtkDataRepresentation> > Representations;
};

svtkStandardNewMacro(svtkView);
//----------------------------------------------------------------------------
svtkView::svtkView()
{
  this->Internal = new svtkView::svtkInternal();
  this->Implementation = new svtkView::svtkImplementation();
  this->Observer = svtkView::Command::New();
  this->Observer->SetTarget(this);
  this->ReuseSingleRepresentation = false;

  // Apply default theme
  svtkViewTheme* theme = svtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
}

//----------------------------------------------------------------------------
svtkView::~svtkView()
{
  this->RemoveAllRepresentations();

  this->Observer->SetTarget(nullptr);
  this->Observer->Delete();
  delete this->Internal;
  delete this->Implementation;
}

//----------------------------------------------------------------------------
bool svtkView::IsRepresentationPresent(svtkDataRepresentation* rep)
{
  unsigned int i;
  for (i = 0; i < this->Implementation->Representations.size(); i++)
  {
    if (this->Implementation->Representations[i] == rep)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkView::AddRepresentationFromInput(svtkDataObject* input)
{
  svtkSmartPointer<svtkTrivialProducer> tp = svtkSmartPointer<svtkTrivialProducer>::New();
  tp->SetOutput(input);
  return this->AddRepresentationFromInputConnection(tp->GetOutputPort());
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkView::SetRepresentationFromInput(svtkDataObject* input)
{
  svtkSmartPointer<svtkTrivialProducer> tp = svtkSmartPointer<svtkTrivialProducer>::New();
  tp->SetOutput(input);
  return this->SetRepresentationFromInputConnection(tp->GetOutputPort());
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkView::CreateDefaultRepresentation(svtkAlgorithmOutput* conn)
{
  svtkDataRepresentation* rep = svtkDataRepresentation::New();
  rep->SetInputConnection(conn);
  return rep;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkView::AddRepresentationFromInputConnection(svtkAlgorithmOutput* conn)
{
  if (this->ReuseSingleRepresentation && this->GetNumberOfRepresentations() > 0)
  {
    this->GetRepresentation()->SetInputConnection(conn);
    return this->GetRepresentation();
  }
  svtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
  {
    svtkErrorMacro("Could not add representation from input connection because "
                  "no default representation was created for the given input connection.");
    return nullptr;
  }

  this->AddRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkView::SetRepresentationFromInputConnection(svtkAlgorithmOutput* conn)
{
  if (this->ReuseSingleRepresentation && this->GetNumberOfRepresentations() > 0)
  {
    this->GetRepresentation()->SetInputConnection(conn);
    return this->GetRepresentation();
  }
  svtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
  {
    svtkErrorMacro("Could not add representation from input connection because "
                  "no default representation was created for the given input connection.");
    return nullptr;
  }

  this->SetRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
void svtkView::AddRepresentation(svtkDataRepresentation* rep)
{
  if (rep != nullptr && !this->IsRepresentationPresent(rep))
  {
    // We add the representation to the internal data-structure before calling
    // AddToView(). This ensures that if the `rep` itself calls
    // AddRepresentation() for an internal representation, the internal
    // representation gets added after the `rep` which makes more sense as it
    // preserves the order for representations in which AddRepresentation() was
    // called.
    size_t index = this->Implementation->Representations.size();
    this->Implementation->Representations.push_back(rep);
    if (rep->AddToView(this))
    {
      rep->AddObserver(svtkCommand::SelectionChangedEvent, this->GetObserver());

      // UpdateEvent is called from push pipeline executions from
      // svtkExecutionScheduler. We want to automatically render the view
      // when one of our representations is updated.
      rep->AddObserver(svtkCommand::UpdateEvent, this->GetObserver());

      this->AddRepresentationInternal(rep);
    }
    else
    {
      this->Implementation->Representations.erase(
        this->Implementation->Representations.begin() + index);
    }
  }
}

//----------------------------------------------------------------------------
void svtkView::SetRepresentation(svtkDataRepresentation* rep)
{
  this->RemoveAllRepresentations();
  this->AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void svtkView::RemoveRepresentation(svtkDataRepresentation* rep)
{
  if (this->IsRepresentationPresent(rep))
  {
    rep->RemoveFromView(this);
    rep->RemoveObserver(this->GetObserver());
    this->RemoveRepresentationInternal(rep);
    std::vector<svtkSmartPointer<svtkDataRepresentation> >::iterator it, itEnd;
    it = this->Implementation->Representations.begin();
    itEnd = this->Implementation->Representations.end();
    for (; it != itEnd; ++it)
    {
      if (it->GetPointer() == rep)
      {
        this->Implementation->Representations.erase(it);
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkView::RemoveRepresentation(svtkAlgorithmOutput* conn)
{
  unsigned int i;
  for (i = 0; i < this->Implementation->Representations.size(); i++)
  {
    svtkDataRepresentation* rep = this->Implementation->Representations[i];
    if (rep->GetNumberOfInputPorts() > 0 && rep->GetInputConnection() == conn)
    {
      this->RemoveRepresentation(rep);
    }
  }
}

//----------------------------------------------------------------------------
void svtkView::RemoveAllRepresentations()
{
  while (!this->Implementation->Representations.empty())
  {
    svtkDataRepresentation* rep = this->Implementation->Representations.back();
    this->RemoveRepresentation(rep);
  }
}

//----------------------------------------------------------------------------
int svtkView::GetNumberOfRepresentations()
{
  return static_cast<int>(this->Implementation->Representations.size());
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkView::GetRepresentation(int index)
{
  if (index >= 0 && index < this->GetNumberOfRepresentations())
  {
    return this->Implementation->Representations[index];
  }
  return nullptr;
}

//----------------------------------------------------------------------------
svtkCommand* svtkView::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void svtkView::ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData)
{
  svtkDataRepresentation* caller_rep = svtkDataRepresentation::SafeDownCast(caller);
  if (this->IsRepresentationPresent(caller_rep) && eventId == svtkCommand::SelectionChangedEvent)
  {
    this->InvokeEvent(svtkCommand::SelectionChangedEvent);
    return;
  }

  if (this->IsRepresentationPresent(caller_rep) && eventId == svtkCommand::UpdateEvent)
  {
    // UpdateEvent is called from push pipeline executions from
    // svtkExecutionScheduler. We want to automatically render the view
    // when one of our representations is updated.
    this->Update();
    return;
  }

  if (eventId == svtkCommand::ProgressEvent)
  {
    std::map<svtkObject*, std::string>::iterator iter =
      this->Internal->RegisteredProgress.find(caller);
    if (iter != this->Internal->RegisteredProgress.end())
    {
      ViewProgressEventCallData eventdata(
        iter->second.c_str(), *(reinterpret_cast<const double*>(callData)));
      this->InvokeEvent(svtkCommand::ViewProgressEvent, &eventdata);
    }
  }
}

//----------------------------------------------------------------------------
void svtkView::RegisterProgress(svtkObject* algorithm, const char* message /*=nullptr*/)
{
  if (algorithm &&
    this->Internal->RegisteredProgress.find(algorithm) != this->Internal->RegisteredProgress.end())
  {
    const char* used_message = message ? message : algorithm->GetClassName();
    this->Internal->RegisteredProgress[algorithm] = used_message;
    algorithm->AddObserver(svtkCommand::ProgressEvent, this->Observer);
  }
}

//----------------------------------------------------------------------------
void svtkView::UnRegisterProgress(svtkObject* algorithm)
{
  if (algorithm)
  {
    std::map<svtkObject*, std::string>::iterator iter =
      this->Internal->RegisteredProgress.find(algorithm);
    if (iter != this->Internal->RegisteredProgress.end())
    {
      this->Internal->RegisteredProgress.erase(iter);
      algorithm->RemoveObservers(svtkCommand::ProgressEvent, this->Observer);
    }
  }
}

//----------------------------------------------------------------------------
void svtkView::Update()
{
  unsigned int i;
  for (i = 0; i < this->Implementation->Representations.size(); i++)
  {
    if (this->Implementation->Representations[i])
    {
      this->Implementation->Representations[i]->Update();
    }
  }
}

//----------------------------------------------------------------------------
void svtkView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
