/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParallelResampling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests ParallelResampling.

/*
** This test only builds if MPI is in use
*/
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDebugLeaks.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPResampleFilter.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProcess.h"
#include "svtkRTAnalyticSource.h"
#include "svtkTestUtilities.h"
#include <svtk_mpi.h>

namespace
{

class MyProcess : public svtkProcess
{
public:
  static MyProcess* New();

  virtual void Execute();

  void SetArgs(int anArgc, char* anArgv[]);

protected:
  MyProcess();

  int Argc;
  char** Argv;
};

svtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = nullptr;
}

void MyProcess::SetArgs(int anArgc, char* anArgv[])
{
  this->Argc = anArgc;
  this->Argv = anArgv;
}

void MyProcess::Execute()
{
  this->ReturnValue = 1;
  int numProcs = this->Controller->GetNumberOfProcesses();
  int me = this->Controller->GetLocalProcessId();
  cout << "Nb process found: " << numProcs << endl;

  // Create and execute pipeline
  svtkNew<svtkRTAnalyticSource> wavelet;
  svtkNew<svtkPResampleFilter> sampler;
  svtkNew<svtkDataSetSurfaceFilter> toPolyData;
  svtkNew<svtkPolyDataMapper> mapper;

  sampler->SetInputConnection(wavelet->GetOutputPort());
  sampler->SetSamplingDimension(21, 21, 21); // 21 for perfect match with wavelet default extent
  // sampler->SetUseInputBounds(0);
  // sampler->SetCustomSamplingBounds(-10, 10, -10, 10, -10, 10);

  toPolyData->SetInputConnection(sampler->GetOutputPort());

  mapper->SetInputConnection(toPolyData->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  cout << "Got for Wavelet " << wavelet->GetOutput()->GetNumberOfPoints() << " points on process "
       << me << endl;
  cout << "Got for Surface " << toPolyData->GetOutput()->GetNumberOfPoints()
       << " points on process " << me << endl;

  if (me == 0)
  {
    // Only root node compare the standard Wavelet data with the probed one
    svtkNew<svtkRTAnalyticSource> waveletBase1Piece;
    waveletBase1Piece->Update();
    svtkImageData* reference = waveletBase1Piece->GetOutput();
    svtkImageData* result = sampler->GetOutput();

    // Compare RTData Array
    svtkFloatArray* rtDataRef =
      svtkArrayDownCast<svtkFloatArray>(reference->GetPointData()->GetArray("RTData"));
    svtkFloatArray* rtDataTest =
      svtkArrayDownCast<svtkFloatArray>(result->GetPointData()->GetArray("RTData"));
    svtkIdType sizeRef = rtDataRef->GetNumberOfTuples();
    if (sizeRef == rtDataTest->GetNumberOfTuples() && rtDataRef->GetNumberOfComponents() == 1)
    {
      for (svtkIdType idx = 0; idx < sizeRef; ++idx)
      {
        if (rtDataRef->GetValue(idx) != rtDataTest->GetValue(idx))
        {
          this->ReturnValue = 0;
          return;
        }
      }
      return; // OK
    }

    this->ReturnValue = 0;
  }
  else
  {
    if (sampler->GetOutput()->GetNumberOfPoints() != 0 ||
      wavelet->GetOutput()->GetNumberOfPoints() == 0)
    {
      this->ReturnValue = 0;
    }
  }
}

}

int ParallelResampling(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1;

  svtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();

  if (!contr->IsA("svtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal; // is this the right error val?   TODO
  }

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();
  p->Delete();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
