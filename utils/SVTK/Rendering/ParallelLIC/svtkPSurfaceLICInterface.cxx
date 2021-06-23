/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSurfaceLICInterface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPSurfaceLICInterface.h"

#include "svtkMPI.h"
#include "svtkObjectFactory.h"
#include "svtkPPainterCommunicator.h"
#include "svtkPainterCommunicator.h"
#include "svtkParallelTimer.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPSurfaceLICInterface);

//----------------------------------------------------------------------------
svtkPSurfaceLICInterface::svtkPSurfaceLICInterface() {}

//----------------------------------------------------------------------------
svtkPSurfaceLICInterface::~svtkPSurfaceLICInterface()
{
#ifdef svtkPSurfaceLICInterfaceDEBUG
  cerr << "=====svtkPSurfaceLICInterface::~svtkPSurfaceLICInterface" << endl;
#endif
}

//----------------------------------------------------------------------------
bool svtkPSurfaceLICInterface::NeedToUpdateCommunicator()
{
  // TODO -- with slice widget in PV the input dataset
  // MTime is changing at different rates on different
  // MPI ranks. Because of this some ranks want to update
  // there comunicator while other do not. To work around
  // this force the communicator update on all ranks if any
  // rank will update it.

  int updateComm = this->Superclass::NeedToUpdateCommunicator() ? 1 : 0;

  svtkMPICommunicatorOpaqueComm* globalComm = svtkPPainterCommunicator::GetGlobalCommunicator();

  if (globalComm)
  {
    MPI_Allreduce(MPI_IN_PLACE, &updateComm, 1, MPI_INT, MPI_MAX, *globalComm->GetHandle());

    if (updateComm != 0)
    {
      this->SetUpdateAll();
    }
  }

  return updateComm != 0;
}

// ----------------------------------------------------------------------------
void svtkPSurfaceLICInterface::GetGlobalMinMax(
  svtkPainterCommunicator* painterComm, float& min, float& max)
{
  svtkPPainterCommunicator* pPainterComm = dynamic_cast<svtkPPainterCommunicator*>(painterComm);

  if (pPainterComm->GetMPIInitialized())
  {
    MPI_Comm comm = *static_cast<MPI_Comm*>(pPainterComm->GetCommunicator());

    MPI_Allreduce(MPI_IN_PLACE, &min, 1, MPI_FLOAT, MPI_MIN, comm);

    MPI_Allreduce(MPI_IN_PLACE, &max, 1, MPI_FLOAT, MPI_MAX, comm);
  }
}

//-----------------------------------------------------------------------------
void svtkPSurfaceLICInterface::StartTimerEvent(const char* event)
{
#if defined(svtkSurfaceLICInterfaceTIME)
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
  log->StartEvent(event);
#else
  (void)event;
#endif
}

//-----------------------------------------------------------------------------
void svtkPSurfaceLICInterface::EndTimerEvent(const char* event)
{
#if defined(svtkSurfaceLICInterfaceTIME)
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
  log->EndEvent(event);
#else
  (void)event;
#endif
}

//----------------------------------------------------------------------------
void svtkPSurfaceLICInterface::WriteTimerLog(const char* fileName)
{
#if defined(svtkSurfaceLICInterfaceTIME)
  std::string fname = fileName ? fileName : "";
  if (fname == this->LogFileName)
  {
    return;
  }
  this->LogFileName = fname;
  if (!fname.empty())
  {
    svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
    log->SetFileName(fname.c_str());
    log->Update();
    log->Write();
  }
#else
  (void)fileName;
#endif
}

//----------------------------------------------------------------------------
svtkPainterCommunicator* svtkPSurfaceLICInterface::CreateCommunicator(int include)
{
  // if we're using MPI and it's been initialized then
  // subset SVTK's world communicator otherwise run the
  // painter serially.
  svtkPPainterCommunicator* comm = new svtkPPainterCommunicator;

  svtkMPICommunicatorOpaqueComm* globalComm = svtkPPainterCommunicator::GetGlobalCommunicator();

  if (globalComm)
  {
    comm->SubsetCommunicator(globalComm, include);
  }

  return comm;
}

//----------------------------------------------------------------------------
void svtkPSurfaceLICInterface::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogFileName=" << this->LogFileName << endl;
}
