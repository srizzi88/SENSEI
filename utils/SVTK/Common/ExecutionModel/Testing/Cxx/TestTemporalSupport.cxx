/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalSupport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAbstractArray.h"
#include "svtkAlgorithm.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include <cassert>

#define CHECK(b, errors)                                                                           \
  if (!(b))                                                                                        \
  {                                                                                                \
    errors++;                                                                                      \
    cerr << "Error on Line " << __LINE__ << ":" << endl;                                           \
  }

using namespace std;
class TestAlgorithm : public svtkAlgorithm
{
public:
  static TestAlgorithm* New();
  svtkTypeMacro(TestAlgorithm, svtkAlgorithm);

  svtkGetMacro(NumRequestInformation, int);
  svtkGetMacro(NumRequestData, int);
  svtkGetMacro(NumRequestUpdateExtent, int);
  svtkGetMacro(NumRequestUpdateTime, int);
  svtkGetMacro(NumRequestTimeDependentInformation, int);

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
    if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      this->NumRequestInformation++;
      return this->RequestInformation(request, inputVector, outputVector);
    }
    if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      this->NumRequestUpdateExtent++;
      return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
    if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
    {
      this->NumRequestData++;
      return this->RequestData(request, inputVector, outputVector);
    }
    if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
    {
      this->NumRequestUpdateTime++;
      return this->RequestUpdateTime(request, inputVector, outputVector);
    }
    if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_TIME_DEPENDENT_INFORMATION()))
    {
      this->NumRequestTimeDependentInformation++;
      return this->RequestTimeDependentInformation(request, inputVector, outputVector);
    }
    return 1;
  }

protected:
  TestAlgorithm()
  {
    this->NumRequestInformation = 0;
    this->NumRequestData = 0;
    this->NumRequestUpdateExtent = 0;
    this->NumRequestUpdateTime = 0;
    this->NumRequestTimeDependentInformation = 0;
  }
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateTime(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestTimeDependentInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  int NumRequestInformation;
  int NumRequestData;
  int NumRequestUpdateExtent;
  int NumRequestUpdateTime;
  int NumRequestTimeDependentInformation;
};
svtkStandardNewMacro(TestAlgorithm);

class TestTimeSource : public TestAlgorithm
{
public:
  static TestTimeSource* New();
  svtkTypeMacro(TestTimeSource, TestAlgorithm);
  svtkSetMacro(HasTimeDependentData, bool);

  TestTimeSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    for (int i = 0; i < 10; i++)
    {
      this->TimeSteps.push_back(i);
    }
    this->HasTimeDependentData = false;
  }

  int RequestData(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector) override
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkImageData* outImage = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
    double timeStep = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    outImage->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), timeStep);
    int scalarType = svtkImageData::GetScalarType(outInfo);
    int numComponents = svtkImageData::GetNumberOfScalarComponents(outInfo);
    outImage->AllocateScalars(scalarType, numComponents);
    return 1;
  }

  int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector) override
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    double range[2] = { 0, 9 };
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &TimeSteps[0],
      static_cast<int>(TimeSteps.size()));
    if (this->HasTimeDependentData)
    {
      outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    }
    return 1;
  }

  int FillOutputPortInformation(int, svtkInformation* info) override
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    return 1;
  }

private:
  vector<double> TimeSteps;
  bool HasTimeDependentData;
  TestTimeSource(const TestTimeSource&) = delete;
  void operator=(const TestTimeSource&) = delete;
};
svtkStandardNewMacro(TestTimeSource);

class TestTimeFilter : public TestAlgorithm
{
public:
  svtkTypeMacro(TestTimeFilter, TestAlgorithm);
  static TestTimeFilter* New();

  svtkSetMacro(StartTime, double);
  svtkSetMacro(TimeIterations, int);

  void PrintSelf(ostream&, svtkIndent) override {}

  int FillInputPortInformation(int, svtkInformation* info) override
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
    return 1;
  }

  int FillOutputPortInformation(int, svtkInformation* info) override
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    return 1;
  }

  int RequestData(
    svtkInformation* request, svtkInformationVector** in, svtkInformationVector*) override
  {
    cout << "Has TD: "
         << in[0]->GetInformationObject(0)->Get(
              svtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION())
         << endl;
    this->TimeIndex++;
    if (this->TimeIndex < this->TimeIterations)
    {
      request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
    else
    {
      this->TimeIndex = 0;
      request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    }
    return 1;
  }

  int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*) override
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    double timeStep = this->StartTime + (double)this->TimeIndex;
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);
    return 1;
  }

private:
  TestTimeFilter()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
    this->StartTime = 0;
    this->TimeIndex = 0;
    this->TimeIterations = 2;
  }
  double StartTime;
  int TimeIndex;
  int TimeIterations;
  TestTimeFilter(const TestTimeFilter&) = delete;
  void operator=(const TestTimeFilter&) = delete;
};
svtkStandardNewMacro(TestTimeFilter);

int TestTimeDependentInformationExecution()
{
  int numErrors(0);
  for (int i = 1; i < 2; i++)
  {
    bool hasTemporalMeta = i == 0 ? false : true;
    svtkNew<TestTimeSource> imageSource;
    imageSource->SetHasTimeDependentData(hasTemporalMeta);

    svtkNew<TestTimeFilter> filter;
    filter->SetTimeIterations(1);
    filter->SetInputConnection(imageSource->GetOutputPort());

    filter->SetStartTime(2.0);
    filter->Update();

    CHECK(imageSource->GetNumRequestData() == 1, numErrors);
    CHECK(imageSource->GetNumRequestInformation() == 1, numErrors);
    CHECK(imageSource->GetNumRequestUpdateExtent() == 1, numErrors);
    if (hasTemporalMeta)
    {
      CHECK(imageSource->GetNumRequestTimeDependentInformation() == 1, numErrors);
      CHECK(filter->GetNumRequestUpdateTime() == 1, numErrors);
    }
    else
    {
      CHECK(imageSource->GetNumRequestTimeDependentInformation() == 0, numErrors);
      CHECK(filter->GetNumRequestUpdateTime() == 0, numErrors);
    }

    filter->SetStartTime(3.0);
    filter->Update(0);
    double dataTime =
      imageSource->GetOutputDataObject(0)->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
    CHECK(dataTime == 3.0, numErrors);
  }

  return numErrors;
}

int TestContinueExecution()
{
  int numErrors(0);
  svtkSmartPointer<TestTimeSource> imageSource = svtkSmartPointer<TestTimeSource>::New();
  svtkSmartPointer<TestTimeFilter> filter = svtkSmartPointer<TestTimeFilter>::New();
  filter->SetInputConnection(imageSource->GetOutputPort());

  int numSteps = 3;
  for (int t = 0; t < numSteps; t++)
  {
    filter->SetStartTime(t);
    filter->Update();
  }
  CHECK(imageSource->GetNumRequestData() == numSteps + 1, numErrors);
  return numErrors;
}

int TestTemporalSupport(int, char*[])
{
  int totalErrors(0);
  int errors(0);
  if ((errors = TestTimeDependentInformationExecution()) != 0)
  {
    totalErrors += errors;
    cerr << errors << " errors in TestTimeDependentInformationExecution" << endl;
  }
  if ((errors = TestContinueExecution()) != 0)
  {
    totalErrors += errors;
    cerr << errors << " errors in TestContinueExecution" << endl;
  }
  return totalErrors;
}
