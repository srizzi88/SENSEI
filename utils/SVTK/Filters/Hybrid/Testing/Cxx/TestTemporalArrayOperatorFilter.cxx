/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalArrayOperatorFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkDoubleArray.h>
#include <svtkImageData.h>
#include <svtkInformation.h>
#include <svtkInformationVector.h>
#include <svtkMathUtilities.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkRTAnalyticSource.h>
#include <svtkStreamingDemandDrivenPipeline.h>
#include <svtkTemporalArrayOperatorFilter.h>

class svtkTemporalRTAnalyticSource : public svtkRTAnalyticSource
{
public:
  static svtkTemporalRTAnalyticSource* New();
  svtkTypeMacro(svtkTemporalRTAnalyticSource, svtkRTAnalyticSource);

protected:
  svtkTemporalRTAnalyticSource() = default;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
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
          timeArray->SetValue(cnt, (1 + t) * idxX + t);
        }
      }
    }
  }

private:
  svtkTemporalRTAnalyticSource(const svtkTemporalRTAnalyticSource&) = delete;
  void operator=(const svtkTemporalRTAnalyticSource&) = delete;
};

svtkStandardNewMacro(svtkTemporalRTAnalyticSource);

//------------------------------------------------------------------------------
// Program main
int TestTemporalArrayOperatorFilter(int, char*[])
{
  svtkNew<svtkTemporalRTAnalyticSource> wavelet;

  // Test ADD operation and default suffix name
  svtkNew<svtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(wavelet->GetOutputPort());

  operatorFilter->SetFirstTimeStepIndex(3);
  operatorFilter->SetSecondTimeStepIndex(0);
  operatorFilter->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "timeData");
  operatorFilter->SetOperator(svtkTemporalArrayOperatorFilter::ADD);

  operatorFilter->UpdateInformation();
  operatorFilter->GetOutputInformation(0)->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 2);
  operatorFilter->Update();

  svtkDataSet* diff = svtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  double range[2];
  diff->GetPointData()->GetArray("timeData")->GetRange(range);
  if (range[0] != 0 && range[1] != 20)
  {
    std::cerr << "Bad initial range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  if (!diff->GetPointData()->GetArray("timeData_add"))
  {
    std::cerr << "Missing 'add' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_add")->GetRange(range);
  if (range[0] != 3 || range[1] != 103)
  {
    std::cerr << "Bad 'sub' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  // Test SUB operation and suffix name
  operatorFilter->SetOperator(svtkTemporalArrayOperatorFilter::SUB);
  operatorFilter->SetOutputArrayNameSuffix("_diff");
  operatorFilter->Update();
  diff = svtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  if (!diff->GetPointData()->GetArray("timeData_diff"))
  {
    std::cerr << "Missing 'sub' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_diff")->GetRange(range);
  if (range[0] != 3 || range[1] != 63)
  {
    std::cerr << "Bad 'sub' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  // Test MUL operation and suffix name
  operatorFilter->SetOperator(svtkTemporalArrayOperatorFilter::MUL);
  operatorFilter->SetOutputArrayNameSuffix("_mul");
  operatorFilter->Update();
  diff = svtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  if (!diff->GetPointData()->GetArray("timeData_mul"))
  {
    std::cerr << "Missing 'mul' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_mul")->GetRange(range);
  if (range[0] != 0 || range[1] != 1660)
  {
    std::cerr << "Bad 'mul' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  // Test DIV operation and default suffix name
  operatorFilter->SetFirstTimeStepIndex(0);
  operatorFilter->SetSecondTimeStepIndex(4);
  operatorFilter->SetOperator(svtkTemporalArrayOperatorFilter::DIV);
  operatorFilter->SetOutputArrayNameSuffix("");
  operatorFilter->Update();
  diff = svtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  if (!diff->GetPointData()->GetArray("timeData_div"))
  {
    std::cerr << "Missing 'div' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_div")->GetRange(range);
  if (range[0] != 0 || svtkMathUtilities::FuzzyCompare(range[1], 0.192308))
  {
    std::cerr << "Bad 'div' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
