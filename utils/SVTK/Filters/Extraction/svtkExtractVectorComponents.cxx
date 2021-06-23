/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractVectorComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractVectorComponents.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkExtractVectorComponents);

svtkExtractVectorComponents::svtkExtractVectorComponents()
{
  this->ExtractToFieldData = 0;
  this->SetNumberOfOutputPorts(3);
  this->OutputsInitialized = 0;
}

svtkExtractVectorComponents::~svtkExtractVectorComponents() = default;

// Get the output dataset representing velocity x-component. If output is nullptr
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 0.)
svtkDataSet* svtkExtractVectorComponents::GetVxComponent()
{
  return this->GetOutput(0);
}

// Get the output dataset representing velocity y-component. If output is nullptr
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 1.)
svtkDataSet* svtkExtractVectorComponents::GetVyComponent()
{
  return this->GetOutput(1);
}

// Get the output dataset representing velocity z-component. If output is nullptr
// then input hasn't been set, which is necessary for abstract objects. (Note:
// this method returns the same information as the GetOutput() method with an
// index of 2.)
svtkDataSet* svtkExtractVectorComponents::GetVzComponent()
{
  return this->GetOutput(2);
}

// Specify the input data or filter.
void svtkExtractVectorComponents::SetInputData(svtkDataSet* input)
{
  if (this->GetNumberOfInputConnections(0) > 0 && this->GetInput(0) == input)
  {
    return;
  }

  this->Superclass::SetInputData(0, input);

  if (input == nullptr)
  {
    return;
  }

  svtkDataSet* output;
  if (!this->OutputsInitialized)
  {
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(1, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(2, output);
    output->Delete();
    this->OutputsInitialized = 1;
    return;
  }

  // since the input has changed we might need to create a new output
  // It seems that output 0 is the correct type as a result of the call to
  // the superclass's SetInput.  Check the type of output 1 instead.
  if (strcmp(this->GetOutput(1)->GetClassName(), input->GetClassName()))
  {
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(1, output);
    output->Delete();
    output = input->NewInstance();
    this->GetExecutive()->SetOutputData(2, output);
    output->Delete();
    svtkWarningMacro(<< " a new output had to be created since the input type changed.");
  }
}
namespace
{

struct svtkExtractComponents
{
  template <class T>
  void operator()(T* vectors, svtkDataArray* vx, svtkDataArray* vy, svtkDataArray* vz)
  {
    T* x = T::FastDownCast(vx);
    T* y = T::FastDownCast(vy);
    T* z = T::FastDownCast(vz);

    const auto inRange = svtk::DataArrayTupleRange<3>(vectors);
    // mark out ranges as single component for better perf
    auto outX = svtk::DataArrayValueRange<1>(x).begin();
    auto outY = svtk::DataArrayValueRange<1>(y).begin();
    auto outZ = svtk::DataArrayValueRange<1>(z).begin();

    for (auto value : inRange)
    {
      *outX++ = value[0];
      *outY++ = value[1];
      *outZ++ = value[2];
    }
  }
};
} // namespace

int svtkExtractVectorComponents::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numVectors = 0, numVectorsc = 0;
  svtkDataArray *vectors, *vectorsc;
  svtkDataArray *vx, *vy, *vz;
  svtkDataArray *vxc, *vyc, *vzc;
  svtkPointData *pd, *outVx, *outVy = nullptr, *outVz = nullptr;
  svtkCellData *cd, *outVxc, *outVyc = nullptr, *outVzc = nullptr;

  svtkDebugMacro(<< "Extracting vector components...");

  // taken out of previous update method.
  output->CopyStructure(input);
  if (!this->ExtractToFieldData)
  {
    this->GetVyComponent()->CopyStructure(input);
    this->GetVzComponent()->CopyStructure(input);
  }

  pd = input->GetPointData();
  cd = input->GetCellData();
  outVx = output->GetPointData();
  outVxc = output->GetCellData();
  if (!this->ExtractToFieldData)
  {
    outVy = this->GetVyComponent()->GetPointData();
    outVz = this->GetVzComponent()->GetPointData();
    outVyc = this->GetVyComponent()->GetCellData();
    outVzc = this->GetVzComponent()->GetCellData();
  }

  vectors = pd->GetVectors();
  vectorsc = cd->GetVectors();
  if ((vectors == nullptr || ((numVectors = vectors->GetNumberOfTuples()) < 1)) &&
    (vectorsc == nullptr || ((numVectorsc = vectorsc->GetNumberOfTuples()) < 1)))
  {
    svtkErrorMacro(<< "No vector data to extract!");
    return 1;
  }

  const char* name;
  if (vectors)
  {
    name = vectors->GetName();
  }
  else if (vectorsc)
  {
    name = vectorsc->GetName();
  }
  else
  {
    name = nullptr;
  }

  size_t newNameSize;
  if (name)
  {
    newNameSize = strlen(name) + 10;
  }
  else
  {
    newNameSize = 10;
    name = "";
  }
  char* newName = new char[newNameSize];

  if (vectors)
  {
    vx = svtkDataArray::CreateDataArray(vectors->GetDataType());
    vx->SetNumberOfTuples(numVectors);
    snprintf(newName, newNameSize, "%s-x", name);
    vx->SetName(newName);
    vy = svtkDataArray::CreateDataArray(vectors->GetDataType());
    vy->SetNumberOfTuples(numVectors);
    snprintf(newName, newNameSize, "%s-y", name);
    vy->SetName(newName);
    vz = svtkDataArray::CreateDataArray(vectors->GetDataType());
    vz->SetNumberOfTuples(numVectors);
    snprintf(newName, newNameSize, "%s-z", name);
    vz->SetName(newName);

    if (!svtkArrayDispatch::Dispatch::Execute(vectors, svtkExtractComponents{}, vx, vy, vz))
    {
      svtkExtractComponents{}(vectors, vx, vy, vz);
    }

    outVx->PassData(pd);
    outVx->AddArray(vx);
    outVx->SetActiveScalars(vx->GetName());
    vx->Delete();

    if (this->ExtractToFieldData)
    {
      outVx->AddArray(vy);
      outVx->AddArray(vz);
    }
    else
    {
      outVy->PassData(pd);
      outVy->AddArray(vy);
      outVy->SetActiveScalars(vy->GetName());

      outVz->PassData(pd);
      outVz->AddArray(vz);
      outVz->SetActiveScalars(vz->GetName());
    }
    vy->Delete();
    vz->Delete();
  }

  if (vectorsc)
  {
    vxc = svtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vxc->SetNumberOfTuples(numVectorsc);
    snprintf(newName, newNameSize, "%s-x", name);
    vxc->SetName(newName);
    vyc = svtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vyc->SetNumberOfTuples(numVectorsc);
    snprintf(newName, newNameSize, "%s-y", name);
    vyc->SetName(newName);
    vzc = svtkDataArray::CreateDataArray(vectorsc->GetDataType());
    vzc->SetNumberOfTuples(numVectorsc);
    snprintf(newName, newNameSize, "%s-z", name);
    vzc->SetName(newName);

    if (!svtkArrayDispatch::Dispatch::Execute(vectorsc, svtkExtractComponents{}, vxc, vyc, vzc))
    {
      svtkExtractComponents{}(vectorsc, vxc, vyc, vzc);
    }

    outVxc->PassData(cd);
    outVxc->AddArray(vxc);
    outVxc->SetActiveScalars(vxc->GetName());
    vxc->Delete();

    if (this->ExtractToFieldData)
    {
      outVxc->AddArray(vyc);
      outVxc->AddArray(vzc);
    }
    else
    {
      outVyc->PassData(cd);
      outVyc->AddArray(vyc);
      outVyc->SetActiveScalars(vyc->GetName());

      outVzc->PassData(cd);
      outVzc->AddArray(vzc);
      outVzc->SetActiveScalars(vzc->GetName());
    }
    vyc->Delete();
    vzc->Delete();
  }
  delete[] newName;

  return 1;
}

void svtkExtractVectorComponents::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ExtractToFieldData: " << this->ExtractToFieldData << endl;
}
