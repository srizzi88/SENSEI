/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestForceTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkActor.h>
#include <svtkDataSetMapper.h>
#include <svtkDataSetTriangleFilter.h>
#include <svtkDoubleArray.h>
#include <svtkForceTime.h>
#include <svtkImageData.h>
#include <svtkInformation.h>
#include <svtkInformationVector.h>
#include <svtkNew.h>
#include <svtkObjectFactory.h>
#include <svtkPointData.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkStreamingDemandDrivenPipeline.h>
#include <svtkUnstructuredGrid.h>

class svtkTimeRTAnalyticSource : public svtkRTAnalyticSource
{
public:
  static svtkTimeRTAnalyticSource* New();
  svtkTypeMacro(svtkTimeRTAnalyticSource, svtkRTAnalyticSource);

protected:
  svtkTimeRTAnalyticSource() = default;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
    // get the info objects
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    double range[2] = { 0, 5 };
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);

    double outTimes[6] = { 0, 1, 2, 3, 4, 5 };
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), outTimes, 6);
    svtkRTAnalyticSource::RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  void ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo) override
  {
    Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    svtkImageData* data = svtkImageData::GetData(outInfo);
    int* outExt = data->GetExtent();

    // find the region to loop over
    int maxX = (outExt[1] - outExt[0]) + 1;
    int maxY = (outExt[3] - outExt[2]) + 1;
    int maxZ = (outExt[5] - outExt[4]) + 1;

    svtkNew<svtkDoubleArray> timeArray;
    timeArray->SetName("timeData");
    timeArray->SetNumberOfValues(maxX * maxY * maxZ);
    data->GetPointData()->SetScalars(timeArray);

    double t = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    svtkIdType cnt = 0;
    for (int idxZ = 0; idxZ < maxZ; idxZ++)
    {
      for (int idxY = 0; idxY < maxY; idxY++)
      {
        for (int idxX = 0; idxX < maxX; idxX++, cnt++)
        {
          timeArray->SetValue(cnt, t + idxX);
        }
      }
    }
  }

private:
  svtkTimeRTAnalyticSource(const svtkTimeRTAnalyticSource&) = delete;
  void operator=(const svtkTimeRTAnalyticSource&) = delete;
};

svtkStandardNewMacro(svtkTimeRTAnalyticSource);

//------------------------------------------------------------------------------
// Program main
int TestForceTime(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the pipeline to produce the initial grid
  svtkNew<svtkTimeRTAnalyticSource> wavelet;
  svtkNew<svtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(wavelet->GetOutputPort());
  svtkNew<svtkForceTime> forceTime;
  forceTime->SetInputConnection(tetrahedralize->GetOutputPort());
  forceTime->SetForcedTime(1);
  forceTime->IgnorePipelineTimeOn();

  forceTime->UpdateInformation();
  forceTime->GetOutputInformation(0)->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 2);
  forceTime->Update();

  if (svtkUnstructuredGrid::SafeDownCast(forceTime->GetOutput(0))
        ->GetPointData()
        ->GetScalars()
        ->GetTuple1(0) != 1)
  {
    std::cerr << "Incorrect data in force time output" << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputConnection(forceTime->GetOutputPort());
  mapper->SetScalarRange(0, 30);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.3, .6, .3); // Background color green

  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
