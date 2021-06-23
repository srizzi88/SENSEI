/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStreamAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestVectorFieldSource.h"
#include "svtkAMRBox.h"
#include "svtkAMREnzoReader.h"
#include "svtkAMRInterpolatedVelocityField.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkInterpolatedVelocityField.h"
#include "svtkMPIController.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPStreamTracer.h"
#include "svtkPoints.h"
#include "svtkPolyDataMapper.h"
#include "svtkTestUtilities.h"
#include "svtkUniformGrid.h"

inline double ComputeLength(svtkIdList* poly, svtkPoints* pts)
{
  int n = poly->GetNumberOfIds();
  if (n == 0)
    return 0;

  double s(0);
  double p[3];
  pts->GetPoint(poly->GetId(0), p);
  for (int j = 1; j < n; j++)
  {
    int pIndex = poly->GetId(j);
    double q[3];
    pts->GetPoint(pIndex, q);
    s += sqrt(svtkMath::Distance2BetweenPoints(p, q));
    memcpy(p, q, 3 * sizeof(double));
  }
  return s;
}

class TestAMRVectorSource : public svtkOverlappingAMRAlgorithm
{
public:
  svtkTypeMacro(TestAMRVectorSource, svtkAlgorithm);
  enum GenerateMethod
  {
    UseVelocity,
    Circular
  };

  svtkSetMacro(Method, GenerateMethod);
  svtkGetMacro(Method, GenerateMethod);

  static TestAMRVectorSource* New();

  virtual int FillInputPortInformation(int, svtkInformation* info) override
  {
    // now add our info
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkOverlappingAMR");
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    return 1;
  }

  GenerateMethod Method;

protected:
  TestAMRVectorSource()
  {
    SetNumberOfInputPorts(1);
    SetNumberOfOutputPorts(1);
  }

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(svtkInformation*, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    svtkInformation* outInfo = outputVector->GetInformationObject(0);

    svtkOverlappingAMR* input =
      svtkOverlappingAMR::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

    svtkOverlappingAMR* output =
      svtkOverlappingAMR::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

    if (!input || !output)
    {
      assert(false);
      return 0;
    }

    output->ShallowCopy(input);

    for (unsigned int level = 0; level < input->GetNumberOfLevels(); ++level)
    {
      for (unsigned int idx = 0; idx < input->GetNumberOfDataSets(level); ++idx)
      {
        svtkUniformGrid* grid = input->GetDataSet(level, idx);
        if (!grid)
        {
          continue;
        }
        svtkCellData* cellData = grid->GetCellData();
        svtkDataArray* xVelocity = cellData->GetArray("x-velocity");
        svtkDataArray* yVelocity = cellData->GetArray("y-velocity");
        svtkDataArray* zVelocity = cellData->GetArray("z-velocity");

        svtkSmartPointer<svtkDoubleArray> velocityVectors = svtkSmartPointer<svtkDoubleArray>::New();
        velocityVectors->SetName("Gradient");
        velocityVectors->SetNumberOfComponents(3);

        int numCells = grid->GetNumberOfCells();
        for (int cellId = 0; cellId < numCells; cellId++)
        {
          assert(xVelocity);
          double velocity[3] = { xVelocity->GetTuple(cellId)[0], yVelocity->GetTuple(cellId)[0],
            zVelocity->GetTuple(cellId)[0] };
          velocityVectors->InsertNextTuple(velocity);
        }
        grid->GetCellData()->AddArray(velocityVectors);
      }
    }

    return 1;
  }
};

svtkStandardNewMacro(TestAMRVectorSource);

int TestPStreamAMR(int argc, char* argv[])
{

  svtkNew<svtkMPIController> c;
  svtkMultiProcessController::SetGlobalController(c);
  c->Initialize(&argc, &argv);
  int numProcs = c->GetNumberOfProcesses();
  int Rank = c->GetLocalProcessId();
  if (numProcs != 4)
  {
    cerr << "Cannot Create four MPI Processes. Success is only norminal";
    return EXIT_SUCCESS;
  }

  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");

  bool res = true;

  double maximumPropagation = 10;
  double stepSize(0.1);

  svtkNew<svtkAMREnzoReader> imageSource;
  imageSource->SetController(c);
  imageSource->SetFileName(fname);
  imageSource->SetMaxLevel(8);
  imageSource->SetCellArrayStatus("x-velocity", 1);
  imageSource->SetCellArrayStatus("y-velocity", 1);
  imageSource->SetCellArrayStatus("z-velocity", 1);

  svtkNew<TestAMRVectorSource> gradientSource;
  gradientSource->SetInputConnection(imageSource->GetOutputPort());

  svtkNew<svtkPStreamTracer> tracer;
  tracer->SetInputConnection(0, gradientSource->GetOutputPort());
  tracer->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "Gradient");
  tracer->SetIntegrationDirection(2);
  tracer->SetIntegratorTypeToRungeKutta4();
  tracer->SetMaximumNumberOfSteps(
    4 * maximumPropagation / stepSize); // shouldn't have to do this fix in stream tracer somewhere!
  tracer->SetMinimumIntegrationStep(stepSize * .1);
  tracer->SetMaximumIntegrationStep(stepSize);
  tracer->SetInitialIntegrationStep(stepSize);

  svtkNew<svtkPolyData> seeds;
  svtkNew<svtkPoints> seedPoints;
  for (double t = 0; t < 1; t += 0.1)
  {
    seedPoints->InsertNextPoint(t, t, t);
  }

  seeds->SetPoints(seedPoints);
  tracer->SetInputData(1, seeds);
  tracer->SetMaximumPropagation(maximumPropagation);

  svtkSmartPointer<svtkPolyDataMapper> traceMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  traceMapper->SetInputConnection(tracer->GetOutputPort());
  traceMapper->SetPiece(Rank);
  traceMapper->SetNumberOfPieces(numProcs);
  traceMapper->Update();

  gradientSource->GetOutputDataObject(0);

  svtkPolyData* out = tracer->GetOutput();

  svtkNew<svtkIdList> polyLine;
  svtkCellArray* lines = out->GetLines();
  double totalLength(0);
  int totalSize(0);
  lines->InitTraversal();
  while (lines->GetNextCell(polyLine))
  {
    double d = ComputeLength(polyLine, out->GetPoints());
    totalLength += d;
    totalSize += polyLine->GetNumberOfIds();
  }
  double totalLengthAll(0);
  c->Reduce(&totalLength, &totalLengthAll, 1, svtkCommunicator::SUM_OP, 0);

  int totalTotalSize(0);
  c->Reduce(&totalSize, &totalTotalSize, 1, svtkCommunicator::SUM_OP, 0);

  if (Rank == 0)
  {
    cout << "Trace Length: " << totalLengthAll << endl;
  }
  res = (totalLengthAll - 17.18) / 17.18 < 0.01;

  c->Finalize();

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
