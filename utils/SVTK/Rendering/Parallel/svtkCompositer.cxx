/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositer.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkToolkits.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkCompositer);

//-------------------------------------------------------------------------
svtkCompositer::svtkCompositer()
{
  this->Controller = svtkMultiProcessController::GetGlobalController();
  this->NumberOfProcesses = 1;
  if (this->Controller)
  {
    this->Controller->Register(this);
    this->NumberOfProcesses = this->Controller->GetNumberOfProcesses();
  }
}

//-------------------------------------------------------------------------
svtkCompositer::~svtkCompositer()
{
  this->SetController(nullptr);
}

//-------------------------------------------------------------------------
void svtkCompositer::SetController(svtkMultiProcessController* mpc)
{
  if (this->Controller == mpc)
  {
    return;
  }
  if (mpc)
  {
    mpc->Register(this);
    this->NumberOfProcesses = mpc->GetNumberOfProcesses();
  }
  if (this->Controller)
  {
    this->Controller->UnRegister(this);
  }
  this->Controller = mpc;
}

//-------------------------------------------------------------------------
void svtkCompositer::CompositeBuffer(
  svtkDataArray* pBuf, svtkFloatArray* zBuf, svtkDataArray* pTmp, svtkFloatArray* zTmp)
{
  (void)pBuf;
  (void)zBuf;
  (void)pTmp;
  (void)zTmp;
}

//-------------------------------------------------------------------------
void svtkCompositer::ResizeFloatArray(svtkFloatArray* fa, int numComp, svtkIdType size)
{
  fa->SetNumberOfComponents(numComp);

#ifdef MPIPROALLOC
  svtkIdType fa_size = fa->GetSize();
  if (fa_size < size * numComp)
  {
    float* ptr = fa->GetPointer(0);
    if (ptr)
    {
      MPI_Free_mem(ptr);
    }
    MPI_Alloc_mem(size * numComp * sizeof(float), nullptr, &ptr);
    fa->SetArray(ptr, size * numComp, 1);
  }
  else
  {
    fa->SetNumberOfTuples(size);
  }
#else
  fa->SetNumberOfTuples(size);
#endif
}

void svtkCompositer::ResizeUnsignedCharArray(svtkUnsignedCharArray* uca, int numComp, svtkIdType size)
{
  uca->SetNumberOfComponents(numComp);
#ifdef MPIPROALLOC
  svtkIdType uca_size = uca->GetSize();

  if (uca_size < size * numComp)
  {
    unsigned char* ptr = uca->GetPointer(0);
    if (ptr)
    {
      MPI_Free_mem(ptr);
    }
    MPI_Alloc_mem(size * numComp * sizeof(unsigned char), nullptr, &ptr);
    uca->SetArray(ptr, size * numComp, 1);
  }
  else
  {
    uca->SetNumberOfTuples(size);
  }
#else
  uca->SetNumberOfTuples(size);
#endif
}

void svtkCompositer::DeleteArray(svtkDataArray* da)
{
#ifdef MPIPROALLOC
  void* ptr = da->GetVoidPointer(0);
  if (ptr)
  {
    MPI_Free_mem(ptr);
  }
#endif
  da->Delete();
}

//-------------------------------------------------------------------------
void svtkCompositer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}
