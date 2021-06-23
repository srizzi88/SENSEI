/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSPHQuinticKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSPHQuinticKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkSPHQuinticKernel);

//----------------------------------------------------------------------------
svtkSPHQuinticKernel::svtkSPHQuinticKernel()
{
  this->CutoffFactor = 3.0;
}

//----------------------------------------------------------------------------
svtkSPHQuinticKernel::~svtkSPHQuinticKernel() = default;

//----------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void svtkSPHQuinticKernel::Initialize(
  svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* attr)
{
  if (this->Dimension == 1)
  {
    this->Sigma = 1.0 / 120.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 7.0 / (478.0 * svtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (120.0 * svtkMath::Pi());
  }

  // Sigma must be set before svtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
void svtkSPHQuinticKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
