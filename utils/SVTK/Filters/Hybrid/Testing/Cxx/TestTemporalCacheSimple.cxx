/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalCacheSimple.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkContourFilter.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTemporalDataSetCache.h"
#include "svtkTemporalInterpolator.h"
#include "svtkThreshold.h"
#include <algorithm>
#include <functional>
#include <vector>

//
// This test is intended to test the ability of the temporal pipeline
// to loop a simple source over T and pass Temporal data downstream.
//

//-------------------------------------------------------------------------
// This is a dummy class which accepts time from the pipeline
// It doesn't do anything with the time, but it is useful for testing
//-------------------------------------------------------------------------
class svtkTemporalSphereSource : public svtkSphereSource
{

public:
  static svtkTemporalSphereSource* New();
  svtkTypeMacro(svtkTemporalSphereSource, svtkSphereSource);

  // Description:
  // Set/Get the time value at which to get the value.
  // These are not used. We get our time from the UPDATE_TIME_STEPS
  // information key
  svtkSetMacro(TimeStep, int);
  svtkGetMacro(TimeStep, int);

  // Save the range of valid timestep index values.
  svtkGetVector2Macro(TimeStepRange, int);

  //  void GetTimeStepValues(std::vector<double> &steps);

protected:
  svtkTemporalSphereSource();

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

public:
  int TimeStepRange[2];
  int TimeStep;
  int ActualTimeStep;
  std::vector<double> TimeStepValues;
};
//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkTemporalSphereSource);
//----------------------------------------------------------------------------
svtkTemporalSphereSource::svtkTemporalSphereSource()
{
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->TimeStep = 0;
  this->ActualTimeStep = 0;
}
//----------------------------------------------------------------------------
int svtkTemporalSphereSource::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  //
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 9;
  this->TimeStepValues.resize(this->TimeStepRange[1] - this->TimeStepRange[0] + 1);
  for (int i = 0; i <= this->TimeStepRange[1]; ++i)
  {
    this->TimeStepValues[i] = i;
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->TimeStepValues[0],
    static_cast<int>(this->TimeStepValues.size()));
  double timeRange[2];
  timeRange[0] = this->TimeStepValues.front();
  timeRange[1] = this->TimeStepValues.back();
  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  return 1;
}
//----------------------------------------------------------------------------
class svtkTestTemporalCacheSimpleWithinTolerance : public std::binary_function<double, double, bool>
{
public:
  result_type operator()(first_argument_type a, second_argument_type b) const
  {
    bool result = (fabs(a - b) <= (a * 1E-6));
    return (result_type)result;
  }
};
//----------------------------------------------------------------------------
int svtkTemporalSphereSource::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* doOutput = outInfo->Get(svtkDataObject::DATA_OBJECT());

  this->ActualTimeStep = this->TimeStep;

  if (this->TimeStep == 0 && outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double requestedTimeValue = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->ActualTimeStep = std::find_if(this->TimeStepValues.begin(), this->TimeStepValues.end(),
                             std::bind(svtkTestTemporalCacheSimpleWithinTolerance(),
                               std::placeholders::_1, requestedTimeValue)) -
      this->TimeStepValues.begin();
    this->ActualTimeStep = this->ActualTimeStep + this->TimeStepRange[0];
  }
  else
  {
    double timevalue;
    timevalue = this->TimeStepValues[this->ActualTimeStep - this->TimeStepRange[0]];
    svtkDebugMacro(<< "Using manually set t= " << timevalue << " Step : " << this->ActualTimeStep);
    doOutput->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), timevalue);
  }

  cout << "this->ActualTimeStep : " << this->ActualTimeStep << endl;

  return Superclass::RequestData(request, inputVector, outputVector);
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
class svtkTestTemporalCacheSimpleExecuteCallback : public svtkCommand
{
public:
  static svtkTestTemporalCacheSimpleExecuteCallback* New()
  {
    return new svtkTestTemporalCacheSimpleExecuteCallback;
  }

  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    // count the number of timesteps requested
    svtkTemporalSphereSource* sph = svtkTemporalSphereSource::SafeDownCast(caller);
    svtkInformation* info = sph->GetExecutive()->GetOutputInformation(0);
    int Length = info->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) ? 1 : 0;
    this->Count += Length;
  }

  unsigned int Count;
};
//-------------------------------------------------------------------------
int TestTemporalCacheSimple(int, char*[])
{
  // test temporal cache with non-temporal data source
  svtkNew<svtkSphereSource> staticSphereSource;
  staticSphereSource->Update();
  svtkPolyData* staticSphere = staticSphereSource->GetOutput();
  svtkNew<svtkTemporalDataSetCache> staticCache;
  staticCache->SetInputConnection(staticSphereSource->GetOutputPort());

  // set a time
  svtkInformation* info = staticCache->GetOutputInformation(0);
  staticCache->UpdateInformation();
  info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 0.0);

  staticCache->Update();
  svtkPolyData* cachedSphere = svtkPolyData::SafeDownCast(staticCache->GetOutputDataObject(0));
  if (staticSphere->GetNumberOfPoints() != cachedSphere->GetNumberOfPoints() ||
    staticSphere->GetNumberOfCells() != cachedSphere->GetNumberOfCells())
  {
    std::cerr << "Cached sphere does not match input sphere" << std::endl;
    return EXIT_FAILURE;
  }

  // create temporal fractals
  svtkSmartPointer<svtkTemporalSphereSource> sphere = svtkSmartPointer<svtkTemporalSphereSource>::New();

  svtkTestTemporalCacheSimpleExecuteCallback* executecb =
    svtkTestTemporalCacheSimpleExecuteCallback::New();
  executecb->Count = 0;
  sphere->AddObserver(svtkCommand::StartEvent, executecb);
  executecb->Delete();

  // cache the data to prevent regenerating some of it
  svtkSmartPointer<svtkTemporalDataSetCache> cache = svtkSmartPointer<svtkTemporalDataSetCache>::New();
  cache->SetInputConnection(sphere->GetOutputPort());
  cache->SetCacheSize(10);

  // interpolate if needed
  svtkSmartPointer<svtkTemporalInterpolator> interp = svtkSmartPointer<svtkTemporalInterpolator>::New();
  // interp->SetInputConnection(fractal->GetOutputPort());
  interp->SetInputConnection(cache->GetOutputPort());

  // map them
  svtkSmartPointer<svtkCompositePolyDataMapper> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper>::New();
  mapper->SetInputConnection(interp->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);

  renWin->AddRenderer(renderer);
  renWin->SetSize(300, 300);
  iren->SetRenderWindow(renWin);

  // ask for some specific data points
  info = interp->GetOutputInformation(0);
  interp->UpdateInformation();
  double time = 0;
  int i;
  int j;
  for (j = 0; j < 5; ++j)
  {
    for (i = 0; i < 9; ++i)
    {
      time = i + 0.5;
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
      mapper->Modified();
      renderer->ResetCameraClippingRange();
      renWin->Render();
    }
  }

  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);

  if (executecb->Count == 11)
  {
    return 0;
  }

  return 1;
}
