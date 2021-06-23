/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMetaData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test verifies that information keys are copied up & down the
// pipeline properly and NeedToExecute/StoreMetaData functions as expected.

#include "svtkInformation.h"
#include "svtkInformationDataObjectMetaDataKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerRequestKey.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataAlgorithm.h"
#include "svtkPolyDataNormals.h"

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

class MySource : public svtkPolyDataAlgorithm
{
public:
  static MySource* New();
  svtkTypeMacro(svtkPolyDataAlgorithm, svtkAlgorithm);

  static svtkInformationDataObjectMetaDataKey* META_DATA();
  static svtkInformationIntegerRequestKey* REQUEST();
  static svtkInformationIntegerKey* DATA();

  bool Failed;
  unsigned int NumberOfExecutions;
  int Result;

protected:
  MySource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    this->Failed = false;
    this->NumberOfExecutions = 0;
    this->Result = -1;
  }

  int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector) override
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkPolyData* pd = svtkPolyData::New();
    outInfo->Set(META_DATA(), pd);
    pd->Delete();
    return 1;
  }
  int RequestData(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector) override
  {
    // Here we verify that a request set at the end of the pipeline
    // made it to here properly.
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (!outInfo->Has(REQUEST()) || outInfo->Get(REQUEST()) != this->Result)
    {
      this->Failed = true;
    }
    this->NumberOfExecutions++;
    return 1;
  }
};

svtkStandardNewMacro(MySource);

svtkInformationKeyMacro(MySource, META_DATA, DataObjectMetaData);
svtkInformationKeyMacro(MySource, DATA, Integer);

class svtkInformationMyRequestKey : public svtkInformationIntegerRequestKey
{
public:
  svtkInformationMyRequestKey(const char* name, const char* location)
    : svtkInformationIntegerRequestKey(name, location)
  {
    this->DataKey = MySource::DATA();
  }
};
svtkInformationKeySubclassMacro(MySource, REQUEST, MyRequest, IntegerRequest);

int TestMetaData(int, char*[])
{
  svtkNew<MySource> mySource;
  svtkNew<svtkPolyDataNormals> filter;

  filter->SetInputConnection(mySource->GetOutputPort());

  filter->UpdateInformation();

  // Do we have the meta-data created by the reader at the end
  // of the pipeline?
  if (!filter->GetOutputInformation(0)->Has(MySource::META_DATA()))
  {
    return TEST_FAILURE;
  }

  filter->GetOutputInformation(0)->Set(MySource::REQUEST(), 2);
  mySource->Result = 2;

  filter->Update();
  // Nothing changed. This should not cause re-execution
  filter->Update();

  filter->GetOutputInformation(0)->Set(MySource::REQUEST(), 3);
  mySource->Result = 3;

  // Request changed. This should cause re-execution
  filter->Update();

  if (mySource->NumberOfExecutions != 2)
  {
    return TEST_FAILURE;
  }

  if (mySource->Failed)
  {
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}
