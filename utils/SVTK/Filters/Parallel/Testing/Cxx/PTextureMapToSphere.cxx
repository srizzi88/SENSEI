/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PTextureMapToSphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests PTextureMapToSphere.

/*
** This test only builds if MPI is in use
*/
#include "svtkActor.h"
#include "svtkCompositeRenderManager.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPNGReader.h"
#include "svtkPTextureMapToSphere.h"
#include "svtkPolyDataMapper.h"
#include "svtkProcess.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkSuperquadricSource.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkTexture.h"
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

  svtkNew<svtkCompositeRenderManager> prm;
  svtkNew<svtkSuperquadricSource> superquadric;
  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPTextureMapToSphere> textureMap;
  svtkNew<svtkPolyDataMapper> mapper;

  superquadric->ToroidalOff();
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);

  // Testing with superquadric which produces processes with no input data
  textureMap->SetInputConnection(superquadric->GetOutputPort());

  mapper->SetInputConnection(textureMap->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetSeamlessU(true);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  // Actually testing in parallel
  textureMap->SetInputConnection(sphere->GetOutputPort());
  mapper->Update();

  char* fname =
    svtkTestUtilities::ExpandDataFileName(this->Argc, this->Argv, "Data/two_svtk_logos_stacked.png");

  svtkNew<svtkPNGReader> PNGReader;
  PNGReader->SetFileName(fname);
  PNGReader->Update();

  svtkNew<svtkTexture> texture;
  texture->SetInputConnection(PNGReader->GetOutputPort());
  texture->InterpolateOn();

  svtkNew<svtkActor> actor;
  actor->SetTexture(texture);
  actor->SetMapper(mapper);

  svtkRenderer* renderer = prm->MakeRenderer();
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.7, 0.7);

  svtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);

  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  prm->InitializeOffScreen(); // Mesa GL only

  constexpr int MY_RETURN_VALUE_MESSAGE = 21545;

  if (me == 0)
  {
    renWin->Render();
    this->ReturnValue = svtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);

    for (int i = 1; i < numProcs; ++i)
    {
      this->Controller->Send(&this->ReturnValue, 1, i, MY_RETURN_VALUE_MESSAGE);
    }
    prm->StopServices();
  }
  else
  {
    prm->StartServices();
    this->Controller->Receive(&this->ReturnValue, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }
  renderer->Delete();
  renWin->Delete();
}

}

int PTextureMapToSphere(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkNew<svtkMPIController> contr;
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
    return retVal; // is this the right error val?   TODO
  }

  svtkNew<MyProcess> p;
  p->SetArgs(argc, argv);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  contr->Finalize();

  return !retVal;
}
