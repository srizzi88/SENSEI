/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXdmf3Parallel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises xdmf3 reading and writing in parallel.
//

#include <svtk_mpi.h>

#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkObjectFactory.h"
#include "svtkProcess.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkXdmf3Reader.h"
#include "svtkXdmf3Writer.h"

#include <svtksys/SystemTools.hxx>

class MyProcess : public svtkProcess
{
public:
  static MyProcess* New();
  svtkTypeMacro(MyProcess, svtkProcess);

  virtual void Execute() override;

  void SetArgs(int argc, char* argv[], const std::string& ifname, const std::string& ofname)
  {
    this->Argc = argc;
    this->Argv = argv;
    this->InFileName = ifname;
    this->OutFileName = ofname;
  }

  void CreatePipeline()
  {
    int num_procs = this->Controller->GetNumberOfProcesses();
    int my_id = this->Controller->GetLocalProcessId();

    this->Reader = svtkXdmf3Reader::New();
    this->Reader->SetFileName(this->InFileName.c_str());
    if (my_id == 0)
    {
      cerr << my_id << "/" << num_procs << endl;
      cerr << "IFILE " << this->InFileName << endl;
      cerr << "OFILE " << this->OutFileName << endl;
    }

    this->Writer = svtkXdmf3Writer::New();
    this->Writer->SetFileName(this->OutFileName.c_str());
    this->Writer->SetInputConnection(this->Reader->GetOutputPort());
  }

protected:
  MyProcess()
  {
    this->Argc = 0;
    this->Argv = nullptr;
  }

  int Argc;
  char** Argv;
  std::string InFileName;
  std::string OutFileName;
  svtkXdmf3Reader* Reader;
  svtkXdmf3Writer* Writer;
};

svtkStandardNewMacro(MyProcess);

void MyProcess::Execute()
{
  int proc = this->Controller->GetLocalProcessId();
  int numprocs = this->Controller->GetNumberOfProcesses();

  this->Controller->Barrier();
  this->CreatePipeline();
  this->Controller->Barrier();
  this->Reader->UpdatePiece(proc, numprocs, 0);
  this->Writer->Write();
  this->Reader->Delete();
  this->Writer->Delete();
  this->ReturnValue = 1;
}

int TestXdmf3Parallel(int argc, char** argv)
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

  int retVal = 1; // 1 == failed

  int numProcs = contr->GetNumberOfProcesses();

  if (numProcs < 2 && false)
  {
    cout << "This test requires at least 2 processes" << endl;
    contr->Delete();
    return retVal;
  }

  svtkMultiProcessController::SetGlobalController(contr);

  svtkTesting* testHelper = svtkTesting::New();
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  std::string datadir = testHelper->GetDataRoot();
  std::string ifile = datadir + "/Data/XDMF/Iron/Iron_Protein.ImageData.xmf";
  std::string tempdir = testHelper->GetTempDirectory();
  tempdir = tempdir + "/XDMF";
  svtksys::SystemTools::MakeDirectory(tempdir.c_str());
  std::string ofile = tempdir + "/Iron_Protein.ImageData.xmf";
  testHelper->Delete();

  // allow caller to use something else
  for (int i = 0; i < argc; i++)
  {
    if (!strncmp(argv[i], "--file=", 11))
    {
      ifile = argv[i] + 11;
    }
  }
  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv, ifile.c_str(), ofile.c_str());

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  p->Delete();
  contr->Finalize();
  contr->Delete();
  svtkMultiProcessController::SetGlobalController(0);

  if (retVal)
  {
    // test passed, remove the files we wrote
    svtksys::SystemTools::RemoveADirectory(tempdir.c_str());
  }
  return !retVal;
}
