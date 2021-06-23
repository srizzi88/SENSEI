/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GenericCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtk_mpi.h>

#include "svtkActor.h"
#include "svtkCallbackCommand.h"
#include "svtkCharArray.h"
#include "svtkContourFilter.h"
#include "svtkDebugLeaks.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkIntArray.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkUnsignedLongArray.h"

#include "svtkDebugLeaks.h"
#include "svtkRegressionTestImage.h"

static const int scMsgLength = 10;

struct GenericCommunicatorArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

void Process2(svtkMultiProcessController* contr, void* svtkNotUsed(arg))
{
  svtkCommunicator* comm = contr->GetCommunicator();

  int i, retVal = 1;

  // Test receiving all supported types of arrays
  svtkIntArray* ia = svtkIntArray::New();
  if (!comm->Receive(ia, 0, 11))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < ia->GetNumberOfTuples(); i++)
  {
    if (ia->GetValue(i) != i)
    {
      cerr << "Server error: Corrupt integer array." << endl;
      retVal = 0;
      break;
    }
  }
  ia->Delete();

  svtkUnsignedLongArray* ula = svtkUnsignedLongArray::New();
  if (!comm->Receive(ula, 0, 22))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < ula->GetNumberOfTuples(); i++)
  {
    if (ula->GetValue(i) != static_cast<unsigned long>(i))
    {
      cerr << "Server error: Corrupt unsigned long array." << endl;
      retVal = 0;
      break;
    }
  }
  ula->Delete();

  svtkCharArray* ca = svtkCharArray::New();
  if (!comm->Receive(ca, 0, 33))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < ca->GetNumberOfTuples(); i++)
  {
    if (ca->GetValue(i) != static_cast<char>(i))
    {
      cerr << "Server error: Corrupt char array." << endl;
      retVal = 0;
      break;
    }
  }
  ca->Delete();

  svtkUnsignedCharArray* uca = svtkUnsignedCharArray::New();
  if (!comm->Receive(uca, 0, 44))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < uca->GetNumberOfTuples(); i++)
  {
    if (uca->GetValue(i) != static_cast<unsigned char>(i))
    {
      cerr << "Server error: Corrupt unsigned char array." << endl;
      retVal = 0;
      break;
    }
  }
  uca->Delete();

  svtkFloatArray* fa = svtkFloatArray::New();
  if (!comm->Receive(fa, 0, 7))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < fa->GetNumberOfTuples(); i++)
  {
    if (fa->GetValue(i) != static_cast<float>(i))
    {
      cerr << "Server error: Corrupt float array." << endl;
      retVal = 0;
      break;
    }
  }
  fa->Delete();

  svtkDoubleArray* da = svtkDoubleArray::New();
  if (!comm->Receive(da, 0, 7))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < da->GetNumberOfTuples(); i++)
  {
    if (da->GetValue(i) != static_cast<double>(i))
    {
      cerr << "Server error: Corrupt double array." << endl;
      retVal = 0;
      break;
    }
  }
  da->Delete();

  svtkIdTypeArray* ita = svtkIdTypeArray::New();
  if (!comm->Receive(ita, 0, 7))
  {
    cerr << "Server error: Error receiving data." << endl;
    retVal = 0;
  }
  for (i = 0; i < ita->GetNumberOfTuples(); i++)
  {
    if (ita->GetValue(i) != static_cast<svtkIdType>(i))
    {
      cerr << "Server error: Corrupt svtkIdType array." << endl;
      retVal = 0;
      break;
    }
  }
  ita->Delete();

  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->Update();
  std::vector<svtkSmartPointer<svtkDataObject> > rdata;
  if (!comm->Gather(sphereSource->GetOutputDataObject(0), rdata, 0))
  {
    cerr << "Server error: Error gathering data." << endl;
    retVal = 0;
  }
  rdata.clear();
  if (!comm->Gather(sphereSource->GetOutputDataObject(0), rdata, 0))
  {
    cerr << "Server error: Error gathering data." << endl;
    retVal = 0;
  }

  comm->Send(&retVal, 1, 0, 11);
}

void Process1(svtkMultiProcessController* contr, void* arg)
{
  GenericCommunicatorArgs_tmp* args = reinterpret_cast<GenericCommunicatorArgs_tmp*>(arg);

  svtkCommunicator* comm = contr->GetCommunicator();

  int i;

  // Test sending all supported types of arrays
  int datai[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datai[i] = i;
  }
  svtkIntArray* ia = svtkIntArray::New();
  ia->SetArray(datai, 10, 1);
  if (!comm->Send(ia, 1, 11))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  ia->Delete();

  unsigned long dataul[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    dataul[i] = static_cast<unsigned long>(i);
  }
  svtkUnsignedLongArray* ula = svtkUnsignedLongArray::New();
  ula->SetArray(dataul, 10, 1);
  if (!comm->Send(ula, 1, 22))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  ula->Delete();

  char datac[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datac[i] = static_cast<char>(i);
  }
  svtkCharArray* ca = svtkCharArray::New();
  ca->SetArray(datac, 10, 1);
  if (!comm->Send(ca, 1, 33))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  ca->Delete();

  unsigned char datauc[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datauc[i] = static_cast<unsigned char>(i);
  }
  svtkUnsignedCharArray* uca = svtkUnsignedCharArray::New();
  uca->SetArray(datauc, 10, 1);
  if (!comm->Send(uca, 1, 44))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  uca->Delete();

  float dataf[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    dataf[i] = static_cast<float>(i);
  }
  svtkFloatArray* fa = svtkFloatArray::New();
  fa->SetArray(dataf, 10, 1);
  if (!comm->Send(fa, 1, 7))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  fa->Delete();

  double datad[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datad[i] = static_cast<double>(i);
  }
  svtkDoubleArray* da = svtkDoubleArray::New();
  da->SetArray(datad, 10, 1);
  if (!comm->Send(da, 1, 7))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  da->Delete();

  svtkIdType datait[scMsgLength];
  for (i = 0; i < scMsgLength; i++)
  {
    datait[i] = static_cast<svtkIdType>(i);
  }
  svtkIdTypeArray* ita = svtkIdTypeArray::New();
  ita->SetArray(datait, 10, 1);
  if (!comm->Send(ita, 1, 7))
  {
    cerr << "Client error: Error sending data." << endl;
    *(args->retVal) = 0;
  }
  ita->Delete();

  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->Update();
  std::vector<svtkSmartPointer<svtkDataObject> > rdata;
  if (!comm->Gather(sphereSource->GetOutputDataObject(0), rdata, 0))
  {
    cerr << "Client error: Error gathering data." << endl;
    *(args->retVal) = 0;
  }
  if (rdata.size() == 2 && svtkPolyData::SafeDownCast(rdata[0]) &&
    svtkPolyData::SafeDownCast(rdata[1]))
  {
  }
  else
  {
    cerr << "Client error: Error gathering data (invalid data received)." << endl;
    *(args->retVal) = 0;
  }
  rdata.clear();
  if (!comm->Gather(nullptr, rdata, 0))
  {
    cerr << "Client error: Error gathering data." << endl;
    *(args->retVal) = 0;
  }
  if (rdata.size() == 2 && rdata[0] == nullptr && svtkPolyData::SafeDownCast(rdata[1]))
  {
  }
  else
  {
    cerr << "Client error: Error gathering data (invalid data received)." << endl;
    *(args->retVal) = 0;
  }

  int remoteRetVal;
  comm->Receive(&remoteRetVal, 1, 1, 11);
  if (!remoteRetVal)
  {
    *(args->retVal) = 0;
  }
}

int GenericCommunicator(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);
  contr->CreateOutputWindow();

  // Added for regression test.
  // ----------------------------------------------
  int retVal = 1;
  GenericCommunicatorArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  contr->SetMultipleMethod(0, Process1, &args);
  contr->SetMultipleMethod(1, Process2, nullptr);
  contr->MultipleMethodExecute();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
