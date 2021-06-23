/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParticleTracers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkParticlePathFilter.h"
#include "svtkParticleTracer.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSetGet.h"
#include "svtkSmartPointer.h"
#include "svtkStreaklineFilter.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include <cassert>
#include <vector>
using namespace std;

class TestTimeSource : public svtkAlgorithm
{
public:
  static TestTimeSource* New();
  svtkTypeMacro(TestTimeSource, svtkAlgorithm);

  svtkGetMacro(NumRequestData, int);

  void SetBoundingBox(double x0, double x1, double y0, double y1, double z0, double z1)
  {
    this->BoundingBox[0] = x0;
    this->BoundingBox[1] = x1;
    this->BoundingBox[2] = y0;
    this->BoundingBox[3] = y1;
    this->BoundingBox[4] = z0;
    this->BoundingBox[5] = z1;
  }
  int GetNumberOfTimeSteps() { return static_cast<int>(this->TimeSteps.size()); }

protected:
  TestTimeSource()
  {
    this->NumRequestData = 0;
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    for (int i = 0; i < 10; i++)
    {
      this->TimeSteps.push_back(i);
    }

    this->Extent[0] = 0;
    this->Extent[1] = 1;
    this->Extent[2] = 0;
    this->Extent[3] = 1;
    this->Extent[4] = 0;
    this->Extent[5] = 1;

    this->BoundingBox[0] = 0;
    this->BoundingBox[1] = 1;
    this->BoundingBox[2] = 0;
    this->BoundingBox[3] = 1;
    this->BoundingBox[4] = 0;
    this->BoundingBox[5] = 1;
  }
  void GetSpacing(double dx[3])
  {
    for (int i = 0; i < 3; i++)
    {
      dx[i] = (this->BoundingBox[2 * i + 1] - this->BoundingBox[2 * i]) /
        (this->Extent[2 * i + 1] - this->Extent[2 * i]);
    }
  }
  ~TestTimeSource() override = default;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
    // generate the data
    if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
    {
      return this->RequestData(request, inputVector, outputVector);
    }

    // execute information
    if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      return this->RequestInformation(request, inputVector, outputVector);
    }
    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
  }

  int FillOutputPortInformation(int, svtkInformation* info) override
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    return 1;
  }

  virtual int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputInfoVector)
  {
    // get the info objects
    svtkInformation* outInfo = outputInfoVector->GetInformationObject(0);

    double range[2] = { 0, 9 };
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);

    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &TimeSteps[0],
      static_cast<int>(TimeSteps.size()));

    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->Extent, 6);

    double spacing[3];
    this->GetSpacing(spacing);

    outInfo->Set(svtkDataObject::SPACING(), spacing[0], spacing[1], spacing[2]);

    double origin[3] = { this->BoundingBox[0], this->BoundingBox[2], this->BoundingBox[4] };
    outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

    return 1;
  }

  int RequestData(svtkInformation*, svtkInformationVector** svtkNotUsed(inputVector),
    svtkInformationVector* outputVector)
  {
    NumRequestData++;
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

    double timeStep = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), timeStep);

    // set the extent to be the update extent
    svtkImageData* outImage = svtkImageData::SafeDownCast(output);
    if (outImage)
    {
      int* uExtent = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

      outImage->SetExtent(uExtent);
      int scalarType = svtkImageData::GetScalarType(outInfo);
      int numComponents = svtkImageData::GetNumberOfScalarComponents(outInfo);
      outImage->AllocateScalars(scalarType, numComponents);
    }
    else
    {
      return 0;
    }

    svtkDataArray* outArray =
      svtkArrayDownCast<svtkDataArray>(svtkAbstractArray::CreateArray(SVTK_FLOAT));
    outArray->SetName("Gradients");
    outArray->SetNumberOfComponents(3);
    outArray->SetNumberOfTuples(outImage->GetNumberOfPoints());
    assert(outArray->GetNumberOfTuples() == outImage->GetNumberOfPoints());
    outImage->GetPointData()->AddArray(outArray);
    outArray->Delete();
    outImage->GetPointData()->SetActiveVectors("Gradients");

    int* extent = outImage->GetExtent(); // old style

    svtkIdType stepX, stepY, stepZ;
    outImage->GetContinuousIncrements(extent, stepX, stepY, stepZ);

    float* outPtr = static_cast<float*>(outImage->GetArrayPointerForExtent(outArray, extent));
    //   PRINT(stepY<<" "<<stepZ)

    int gridSize[3] = { Extent[1] - Extent[0], Extent[3] - Extent[2], Extent[5] - Extent[4] };

    double* origin = outImage->GetOrigin();

    double size[3];
    for (int i = 0; i < 3; i++)
    {
      size[i] = this->BoundingBox[2 * i + 1] - this->BoundingBox[2 * i];
    }

    double speed = 0.1 * timeStep;
    for (int iz = extent[4]; iz <= extent[5]; iz++)
    {
      for (int iy = extent[2]; iy <= extent[3]; iy++)
      {
        for (int ix = extent[0]; ix <= extent[1]; ix++)
        {
          double x = size[0] * ((double)ix) / gridSize[0] + origin[0];
          // double y = size[1]*((double)iy)/gridSize[1] + origin[1];
          double z = size[2] * ((double)iz) / gridSize[2] + origin[2];
          *(outPtr++) = -z * speed;
          *(outPtr++) = 0;
          *(outPtr++) = x * speed;
        }
        outPtr += stepY;
      }
      outPtr += stepZ;
    }
    return 1;
  }

private:
  TestTimeSource(const TestTimeSource&) = delete;
  void operator=(const TestTimeSource&) = delete;

  vector<double> TimeSteps;
  int Extent[6];
  double BoundingBox[6];
  int NumRequestData;
};

svtkStandardNewMacro(TestTimeSource);

#define EXPECT(a, msg)                                                                             \
  if (!(a))                                                                                        \
  {                                                                                                \
    cerr << "Line " << __LINE__ << ":" << msg << endl;                                             \
    return EXIT_FAILURE;                                                                           \
  }

int TestParticlePathFilter()
{
  svtkNew<TestTimeSource> imageSource;
  imageSource->SetBoundingBox(-1, 1, -1, 1, -1, 1);

  svtkNew<svtkPoints> points;
  points->InsertNextPoint(0.5, 0, 0);
  points->InsertNextPoint(0.4, 0, 0);

  svtkNew<svtkPolyData> ps;
  ps->SetPoints(points);

  svtkNew<svtkParticlePathFilter> filter;

  filter->SetInputConnection(0, imageSource->GetOutputPort());
  filter->SetInputData(1, ps);

  filter->SetTerminationTime(3.3);
  filter->Update();

  svtkPolyData* out = filter->GetOutput();
  svtkCellArray* lines = out->GetLines();
  svtkNew<svtkIdList> polyLine;
  //  svtkIntArray* particleIds =
  //  svtkArrayDownCast<svtkIntArray>(out->GetPointData()->GetArray("ParticleId"));

  double baseLines[2] = { 0.271834, 0.217467 };
  int lineIndex(0);

  lines->InitTraversal();
  while (lines->GetNextCell(polyLine))
  {
    double s = 0;
    for (int j = 1; j < polyLine->GetNumberOfIds(); j++)
    {
      int pIndex = polyLine->GetId(j - 1);
      int qIndex = polyLine->GetId(j);
      double p[3], q[3];
      out->GetPoints()->GetPoint(pIndex, p);
      out->GetPoints()->GetPoint(qIndex, q);
      s += sqrt(svtkMath::Distance2BetweenPoints(p, q));
    }
    EXPECT(fabs(s - baseLines[lineIndex++]) < 0.01, "Wrong particle path length " << s);
  }

  int numRequestData = imageSource->GetNumRequestData();
  EXPECT(numRequestData == 5, "Wrong");

  filter->SetTerminationTime(4.0);
  filter->Update();

  numRequestData = imageSource->GetNumRequestData();
  EXPECT(imageSource->GetNumRequestData() == numRequestData, "Wrong # of requests");

  out = filter->GetOutput();
  EXPECT(out->GetNumberOfLines() == 2, "Wrong # of lines" << out->GetNumberOfLines());

  double baseLines1[2] = { 0.399236, 0.319389 };
  lines = out->GetLines();
  lines->InitTraversal();
  lineIndex = 0;
  while (lines->GetNextCell(polyLine))
  {
    double s = 0;
    for (int j = 1; j < polyLine->GetNumberOfIds(); j++)
    {
      int pIndex = polyLine->GetId(j - 1);
      int qIndex = polyLine->GetId(j);
      double p[3], q[3];
      out->GetPoints()->GetPoint(pIndex, p);
      out->GetPoints()->GetPoint(qIndex, q);
      s += sqrt(svtkMath::Distance2BetweenPoints(p, q));
    }
    EXPECT(fabs(s - baseLines1[lineIndex++]) < 0.01, "Wrong particle path length " << s);
  }

  filter->SetTerminationTime(0);
  filter->Update();

  filter->SetTerminationTime(0.2);
  filter->Update();

  return EXIT_SUCCESS;
}

int TestParticlePathFilterStartTime()
{
  svtkNew<TestTimeSource> imageSource;
  imageSource->SetBoundingBox(-1, 1, -1, 1, -1, 1);

  svtkNew<svtkPoints> points;
  points->InsertNextPoint(0.5, 0, 0);

  svtkNew<svtkPolyData> ps;
  ps->SetPoints(points);

  svtkNew<svtkParticlePathFilter> filter;
  filter->SetStartTime(2.0);
  filter->SetInputConnection(0, imageSource->GetOutputPort());
  filter->SetInputData(1, ps);

  filter->SetTerminationTime(5.3);
  filter->Update();

  svtkPolyData* out = filter->GetOutput();
  EXPECT(out->GetNumberOfCells() == 1, "Wrong number of particle paths for non-zero start time");

  svtkCell* cell = out->GetCell(0);
  EXPECT(
    cell->GetNumberOfPoints() == 6, "Wrong number of points for non-zero particle path start time");
  double pt[3];
  out->GetPoint(cell->GetPointId(5), pt);
  EXPECT(fabs(pt[0] - 0.179085) < 0.01 && fabs(pt[1]) < 0.01 && fabs(pt[2] - 0.466826) < 0.01,
    "Wrong end point for particle path with non-zero start time");

  return EXIT_SUCCESS;
}

int TestStreaklineFilter()
{
  svtkNew<TestTimeSource> imageSource;
  imageSource->SetBoundingBox(-1, 1, -1, 1, -1, 1);

  // Test streak line filter
  svtkNew<svtkPoints> points;
  points->InsertNextPoint(0.5, 0, 0);
  points->InsertNextPoint(0.4, 0, 0);
  svtkNew<svtkPolyData> pointsSource;
  pointsSource->SetPoints(points);

  svtkNew<svtkStreaklineFilter> filter;
  filter->SetInputConnection(0, imageSource->GetOutputPort());
  filter->SetInputData(1, pointsSource);

  filter->SetStartTime(0.0);
  filter->SetTerminationTime(3.0);
  filter->Update();

  svtkPolyData* out = filter->GetOutput();
  EXPECT(out->GetNumberOfLines() == 2, "Wrong number of streaks: " << out->GetNumberOfLines());

  svtkCellArray* lines = out->GetLines();
  svtkNew<svtkIdList> polyLine;
  svtkFloatArray* particleAge =
    svtkArrayDownCast<svtkFloatArray>(out->GetPointData()->GetArray("ParticleAge"));

  lines->InitTraversal();
  while (lines->GetNextCell(polyLine))
  {
    for (int j = 1; j < polyLine->GetNumberOfIds(); j++)
    {
      int pIndex = polyLine->GetId(j - 1);
      int qIndex = polyLine->GetId(j);
      EXPECT(particleAge->GetValue(pIndex) > particleAge->GetValue(qIndex), "Wrong point order");
    }
  }

  filter->SetTerminationTime(4.0);
  filter->Update();

  return EXIT_SUCCESS;
}

int TestParticleTracers(int, char*[])
{
  svtkPoints* pts(nullptr);
  double p[3];

  svtkNew<TestTimeSource> imageSource;
  imageSource->SetBoundingBox(-1, 1, -1, 1, -1, 1);
  svtkNew<svtkPointSource> ps;
  ps->SetCenter(0.5, 0., 0.);
  ps->SetRadius(0);
  ps->SetNumberOfPoints(1);

  svtkNew<svtkParticleTracer> filter;
  filter->SetInputConnection(0, imageSource->GetOutputPort());
  filter->SetInputConnection(1, ps->GetOutputPort());
  filter->SetComputeVorticity(0);

  filter->SetStartTime(0.1);
  filter->SetTerminationTime(4.5);
  filter->Update();
  double data_time =
    filter->GetOutputDataObject(0)->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
  int numRequestData = imageSource->GetNumRequestData();
  EXPECT(data_time == 4.5, "Wrong time");

  if (numRequestData != 6)
  {
    cerr << "Wrong num requests\n";
    return EXIT_FAILURE;
  }

  pts = svtkPolyData::SafeDownCast(filter->GetOutputDataObject(0))->GetPoints();
  pts->GetPoint(0, p);
  EXPECT(fabs(p[2] - 0.424) < 0.01, "Wrong termination point")

  filter->SetTerminationTime(5.5);
  filter->Update();
  pts = svtkPolyData::SafeDownCast(filter->GetOutputDataObject(0))->GetPoints();
  pts->GetPoint(0, p);
  EXPECT(fabs(p[2] - 0.499) < 0.01, "Wrong termination point");
  EXPECT(imageSource->GetNumRequestData() - numRequestData == 1, " too many requests");

  numRequestData = imageSource->GetNumRequestData();

  filter->SetStartTime(0.10001);
  filter->Update();
  pts = svtkPolyData::SafeDownCast(filter->GetOutputDataObject(0))->GetPoints();

  double p1[3];
  pts->GetPoint(0, p1);

  EXPECT(fabs(p[2] - p1[2]) < 0.001, "Wrong termination point");
  EXPECT(imageSource->GetNumRequestData() - numRequestData == 7, "Wrong # of requests");
  numRequestData = imageSource->GetNumRequestData();

  ps->SetCenter(999, 999, 999);
  ps->SetCenter(0.5, 0., 0.);

  filter->Update();
  EXPECT(imageSource->GetNumRequestData() - numRequestData == 7, "Wrong # of requests");
  numRequestData = imageSource->GetNumRequestData();

  filter->SetIgnorePipelineTime(1);
  filter->UpdateTimeStep(6.5);

  EXPECT(imageSource->GetNumRequestData() - numRequestData == 0, "Pipeline Time should be ignored")
  numRequestData = imageSource->GetNumRequestData();

  filter->SetIgnorePipelineTime(0);
  filter->Update();
  EXPECT(imageSource->GetNumRequestData() - numRequestData == 1, "Wrong");

  filter->UpdateTimeStep(0.0);
  pts = svtkPolyData::SafeDownCast(filter->GetOutputDataObject(0))->GetPoints();
  EXPECT(pts->GetNumberOfPoints() == 1, "should have points even if start and stop time coincide");

  filter->UpdateTimeStep(100.0);
  filter->UpdateTimeStep(200.0);
  filter->SetIgnorePipelineTime(true);
  filter->SetTerminationTime(9.0); // make sure this doesn't crash
  filter->Update();

  EXPECT(TestParticlePathFilter() == EXIT_SUCCESS, "");
  EXPECT(TestParticlePathFilterStartTime() == EXIT_SUCCESS, "");
  EXPECT(TestStreaklineFilter() == EXIT_SUCCESS, "");

  return EXIT_SUCCESS;
}
