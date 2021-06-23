/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPMaskPoints.h"

#include "svtkCommunicator.h"
#include "svtkDummyController.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPMaskPoints);

//-----------------------------------------------------------------------------
svtkPMaskPoints::svtkPMaskPoints()
{
  this->Controller = nullptr;

  svtkSmartPointer<svtkMultiProcessController> controller =
    svtkMultiProcessController::GetGlobalController();
  if (!controller)
  {
    controller = svtkSmartPointer<svtkDummyController>::New();
  }
  this->SetController(controller);
}

//----------------------------------------------------------------------------
svtkPMaskPoints::~svtkPMaskPoints()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
void svtkPMaskPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->GetController())
  {
    os << indent << "Controller: " << this->GetController() << std::endl;
  }
  else
  {
    os << indent << "Controller: (null)" << std::endl;
  }
}

//----------------------------------------------------------------------------
void svtkPMaskPoints::SetController(svtkMultiProcessController* c)
{
  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller != nullptr)
  {
    this->Controller->UnRegister(this);
    this->Controller = nullptr;
  }

  if (c == nullptr)
  {
    return;
  }

  this->Controller = c;
  c->Register(this);
}

//----------------------------------------------------------------------------
svtkMultiProcessController* svtkPMaskPoints::GetController()
{
  return (svtkMultiProcessController*)this->Controller;
}

//----------------------------------------------------------------------------
void svtkPMaskPoints::InternalScatter(unsigned long* a, unsigned long* b, int c, int d)
{
  this->Controller->Scatter(a, b, c, d);
}

//----------------------------------------------------------------------------
void svtkPMaskPoints::InternalGather(unsigned long* a, unsigned long* b, int c, int d)
{
  this->Controller->Gather(a, b, c, d);
}

//----------------------------------------------------------------------------
int svtkPMaskPoints::InternalGetNumberOfProcesses()
{
  return this->Controller->GetNumberOfProcesses();
}

//----------------------------------------------------------------------------
int svtkPMaskPoints::InternalGetLocalProcessId()
{
  return this->Controller->GetLocalProcessId();
}

//----------------------------------------------------------------------------
void svtkPMaskPoints::InternalBarrier()
{
  this->Controller->Barrier();
}

//----------------------------------------------------------------------------
void svtkPMaskPoints::InternalSplitController(int color, int key)
{
  this->OriginalController = this->Controller;
  this->Controller = this->OriginalController->PartitionController(color, key);
}

//----------------------------------------------------------------------------
void svtkPMaskPoints::InternalResetController()
{
  this->Controller->Delete();
  this->Controller = this->OriginalController;
  this->OriginalController = nullptr;
}
