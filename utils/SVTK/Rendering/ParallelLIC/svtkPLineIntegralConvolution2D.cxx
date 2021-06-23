/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLineIntegralConvolution2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPLineIntegralConvolution2D.h"

#include "svtkMPI.h"
#include "svtkObjectFactory.h"
#include "svtkPPainterCommunicator.h"
#include "svtkPainterCommunicator.h"
#include "svtkParallelTimer.h"

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPLineIntegralConvolution2D);

// ----------------------------------------------------------------------------
svtkPLineIntegralConvolution2D::svtkPLineIntegralConvolution2D()
{
  this->Comm = new svtkPPainterCommunicator;
}

// ----------------------------------------------------------------------------
svtkPLineIntegralConvolution2D::~svtkPLineIntegralConvolution2D() {}

// ----------------------------------------------------------------------------
void svtkPLineIntegralConvolution2D::SetCommunicator(svtkPainterCommunicator* comm)
{
  this->Comm->Copy(comm, false);
}

// ----------------------------------------------------------------------------
svtkPainterCommunicator* svtkPLineIntegralConvolution2D::GetCommunicator()
{
  return this->Comm;
}

// ----------------------------------------------------------------------------
void svtkPLineIntegralConvolution2D::GetGlobalMinMax(
  svtkPainterCommunicator* painterComm, float& min, float& max)
{
  svtkPPainterCommunicator* pPainterComm = dynamic_cast<svtkPPainterCommunicator*>(painterComm);

  if (pPainterComm->GetMPIInitialized())
  {
    MPI_Comm comm = *((MPI_Comm*)pPainterComm->GetCommunicator());

    MPI_Allreduce(MPI_IN_PLACE, &min, 1, MPI_FLOAT, MPI_MIN, comm);

    MPI_Allreduce(MPI_IN_PLACE, &max, 1, MPI_FLOAT, MPI_MAX, comm);
  }
}

//-----------------------------------------------------------------------------
void svtkPLineIntegralConvolution2D::StartTimerEvent(const char* event)
{
#if defined(svtkLineIntegralConvolution2DTIME) || defined(svtkSurfaceLICPainterTIME)
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
  log->StartEvent(event);
#else
  (void)event;
#endif
}

//-----------------------------------------------------------------------------
void svtkPLineIntegralConvolution2D::EndTimerEvent(const char* event)
{
#if defined(svtkLineIntegralConvolution2DTIME) || defined(svtkSurfaceLICPainterTIME)
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();
  log->EndEvent(event);
#else
  (void)event;
#endif
}

//----------------------------------------------------------------------------
void svtkPLineIntegralConvolution2D::WriteTimerLog(const char* fileName)
{
#ifdef svtkLineIntegralConvolution2DTIME
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

//-----------------------------------------------------------------------------
void svtkPLineIntegralConvolution2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogFileName=" << this->LogFileName << endl;
}
