/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkAppendSelection.h"

#include "svtkAlgorithmOutput.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkAppendSelection);

//----------------------------------------------------------------------------
svtkAppendSelection::svtkAppendSelection()
{
  this->UserManagedInputs = 0;
  this->AppendByUnion = 1;
}

//----------------------------------------------------------------------------
svtkAppendSelection::~svtkAppendSelection() = default;

//----------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void svtkAppendSelection::AddInputData(svtkSelection* ds)
{
  if (this->UserManagedInputs)
  {
    svtkErrorMacro(<< "AddInput is not supported if UserManagedInputs is true");
    return;
  }
  this->AddInputDataInternal(0, ds);
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void svtkAppendSelection::RemoveInputData(svtkSelection* ds)
{
  if (this->UserManagedInputs)
  {
    svtkErrorMacro(<< "RemoveInput is not supported if UserManagedInputs is true");
    return;
  }

  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0, this->GetInputConnection(0, i));
    }
  }
}

//----------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void svtkAppendSelection::SetNumberOfInputs(int num)
{
  if (!this->UserManagedInputs)
  {
    svtkErrorMacro(<< "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputConnections(0, num);
}

//----------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void svtkAppendSelection::SetInputConnectionByNumber(int num, svtkAlgorithmOutput* input)
{
  if (!this->UserManagedInputs)
  {
    svtkErrorMacro(<< "SetInputByNumber is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, num, input);
}

//----------------------------------------------------------------------------
int svtkAppendSelection::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  svtkSelection* output = svtkSelection::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  output->Initialize();

  // If there are no inputs, we are done.
  int numInputs = this->GetNumberOfInputConnections(0);
  if (numInputs == 0)
  {
    return 1;
  }

  if (!this->AppendByUnion)
  {
    for (int idx = 0; idx < numInputs; ++idx)
    {
      svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
      svtkSelection* sel = svtkSelection::GetData(inInfo);
      if (sel != nullptr)
      {
        for (unsigned int j = 0; j < sel->GetNumberOfNodes(); ++j)
        {
          svtkSelectionNode* inputNode = sel->GetNode(j);
          svtkSmartPointer<svtkSelectionNode> outputNode = svtkSmartPointer<svtkSelectionNode>::New();
          outputNode->ShallowCopy(inputNode);
          output->AddNode(outputNode);
        }
      }
    } // for each input
    return 1;
  }

  // The first non-null selection determines the required content type of all selections.
  int idx = 0;
  svtkSelection* first = nullptr;
  while (first == nullptr && idx < numInputs)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    first = svtkSelection::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    idx++;
  }

  // If they are all null, return.
  if (first == nullptr)
  {
    return 1;
  }

  output->ShallowCopy(first);

  // Take the union of all non-null selections
  for (; idx < numInputs; ++idx)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    svtkSelection* s = svtkSelection::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (s != nullptr)
    {
      output->Union(s);
    } // for a non nullptr input
  }   // for each input

  return 1;
}

//----------------------------------------------------------------------------
svtkSelection* svtkAppendSelection::GetInput(int idx)
{
  return svtkSelection::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
int svtkAppendSelection::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendSelection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "UserManagedInputs: " << (this->UserManagedInputs ? "On" : "Off") << endl;
  os << "AppendByUnion: " << (this->AppendByUnion ? "On" : "Off") << endl;
}
