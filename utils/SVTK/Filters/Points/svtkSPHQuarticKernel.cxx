/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSPHQuarticKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSPHQuarticKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkSPHQuarticKernel);

//----------------------------------------------------------------------------
svtkSPHQuarticKernel::svtkSPHQuarticKernel()
{
  this->CutoffFactor = 2.5;

  if (this->Dimension == 1)
  {
    this->Sigma = 1.0 / 24.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 96.0 / (1199.0 * svtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (20.0 * svtkMath::Pi());
  }
}

//----------------------------------------------------------------------------
svtkSPHQuarticKernel::~svtkSPHQuarticKernel() = default;

//----------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void svtkSPHQuarticKernel::Initialize(
  svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* attr)
{
  if (this->Dimension == 1)
  {
    this->Sigma = 1.0 / 24.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 96.0 / (1199.0 * svtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (20.0 * svtkMath::Pi());
  }

  // Sigma must be set before svtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
void svtkSPHQuarticKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
