/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnsembleSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEnsembleSource.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationDataObjectMetaDataKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerRequestKey.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

#include <vector>

svtkStandardNewMacro(svtkEnsembleSource);
svtkCxxSetObjectMacro(svtkEnsembleSource, MetaData, svtkTable);

svtkInformationKeyMacro(svtkEnsembleSource, META_DATA, DataObjectMetaData);
svtkInformationKeyMacro(svtkEnsembleSource, DATA_MEMBER, Integer);

// Subclass svtkInformationIntegerRequestKey to set the DataKey.
class svtkInformationEnsembleMemberRequestKey : public svtkInformationIntegerRequestKey
{
public:
  svtkInformationEnsembleMemberRequestKey(const char* name, const char* location)
    : svtkInformationIntegerRequestKey(name, location)
  {
    this->DataKey = svtkEnsembleSource::DATA_MEMBER();
  }
};
svtkInformationKeySubclassMacro(
  svtkEnsembleSource, UPDATE_MEMBER, EnsembleMemberRequest, IntegerRequest);

struct svtkEnsembleSourceInternal
{
  std::vector<svtkSmartPointer<svtkAlgorithm> > Algorithms;
};

svtkEnsembleSource::svtkEnsembleSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internal = new svtkEnsembleSourceInternal;

  this->CurrentMember = 0;

  this->MetaData = nullptr;
}

svtkEnsembleSource::~svtkEnsembleSource()
{
  delete this->Internal;

  if (this->MetaData)
  {
    this->MetaData->Delete();
    this->MetaData = nullptr;
  }
}

svtkTypeBool svtkEnsembleSource::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkAlgorithm* currentReader = this->GetCurrentReader(outInfo);
  if (currentReader)
  {
    if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
      // Make sure to initialize our output to the right type.
      // Note all ensemble members are expected to produce the same
      // data type or we are toast.
      currentReader->UpdateDataObject();
      svtkDataObject* rOutput = currentReader->GetOutputDataObject(0);
      svtkDataObject* output = rOutput->NewInstance();
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      return 1;
    }
    if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      if (this->MetaData)
      {
        outputVector->GetInformationObject(0)->Set(META_DATA(), this->MetaData);
      }
      // Call RequestInformation on all readers as they may initialize
      // data structures there. Note that this has to be done here
      // because current reader can be changed with a pipeline request
      // which does not cause REQUEST_INFORMATION to happen again.
      std::vector<svtkSmartPointer<svtkAlgorithm> >::iterator iter =
        this->Internal->Algorithms.begin();
      std::vector<svtkSmartPointer<svtkAlgorithm> >::iterator end = this->Internal->Algorithms.end();
      for (; iter != end; ++iter)
      {
        int retVal = (*iter)->ProcessRequest(request, inputVector, outputVector);
        if (!retVal)
        {
          return retVal;
        }
      }
      return 1;
    }

    return currentReader->ProcessRequest(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void svtkEnsembleSource::AddMember(svtkAlgorithm* alg)
{
  this->Internal->Algorithms.push_back(alg);
}

void svtkEnsembleSource::RemoveAllMembers()
{
  this->Internal->Algorithms.clear();
}

unsigned int svtkEnsembleSource::GetNumberOfMembers()
{
  return static_cast<unsigned int>(this->Internal->Algorithms.size());
}

svtkAlgorithm* svtkEnsembleSource::GetCurrentReader(svtkInformation* outInfo)
{
  unsigned int currentMember = 0;
  if (outInfo->Has(UPDATE_MEMBER()))
  {
    currentMember = static_cast<unsigned int>(outInfo->Get(UPDATE_MEMBER()));
  }
  else
  {
    currentMember = this->CurrentMember;
  }
  if (currentMember >= this->GetNumberOfMembers())
  {
    return nullptr;
  }
  return this->Internal->Algorithms[currentMember];
}

int svtkEnsembleSource::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

void svtkEnsembleSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Current member: " << this->CurrentMember << endl;
  os << indent << "MetaData: " << endl;
  if (this->MetaData)
  {
    this->MetaData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(nullptr)" << endl;
  }
}
