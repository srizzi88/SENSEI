/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkYoungsMaterialInterface.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm
// contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
// F-91297 Arpajon, France. <br> Implementation by Thierry Carrard and Philippe Pebay

#include "svtkPYoungsMaterialInterface.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPYoungsMaterialInterface);
svtkCxxSetObjectMacro(svtkPYoungsMaterialInterface, Controller, svtkMultiProcessController);
//-----------------------------------------------------------------------------
svtkPYoungsMaterialInterface::svtkPYoungsMaterialInterface()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());

  svtkDebugMacro(<< "svtkPYoungsMaterialInterface::svtkPYoungsMaterialInterface() ok\n");
}

//-----------------------------------------------------------------------------
svtkPYoungsMaterialInterface::~svtkPYoungsMaterialInterface()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
void svtkPYoungsMaterialInterface::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
void svtkPYoungsMaterialInterface::Aggregate(int nmat, int* inputsPerMaterial)
{
  svtkIdType nprocs = this->Controller->GetNumberOfProcesses();
  if (nprocs < 2)
  {
    return;
  }

  // Now get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkErrorMacro(<< "No parallel communicator.");
  }

  // Gather inputs per material from all processes
  svtkIdType myid = this->Controller->GetLocalProcessId();
  int* tmp = new int[nmat * nprocs];
  com->AllGather(inputsPerMaterial, tmp, nmat);

  // Scan sum : done by all processes, not optimal but easy
  for (svtkIdType m = 0; m < nmat; ++m)
  {
    for (svtkIdType p = 1; p < nprocs; ++p)
    {
      svtkIdType pnmat = p * nmat + m;
      tmp[pnmat] += tmp[pnmat - nmat];
    }
  }

  svtkIdType offset = (nprocs - 1) * nmat;
  this->NumberOfDomains = 0;
  for (int m = 0; m < nmat; ++m)
  {
    // Sum all counts from all processes
    int inputsPerMaterialSum = tmp[offset + m];
    if (inputsPerMaterialSum > this->NumberOfDomains)
    {
      this->NumberOfDomains = inputsPerMaterialSum;
    }

    // Calculate partial sum of all preceding processors
    inputsPerMaterial[m] = (myid ? tmp[(myid - 1) * nmat + m] : 0);
  }
  delete[] tmp;
}
