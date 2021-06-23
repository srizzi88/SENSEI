/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSPHCubicKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSPHCubicKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkSPHCubicKernel);

//----------------------------------------------------------------------------
svtkSPHCubicKernel::svtkSPHCubicKernel()
{
  this->CutoffFactor = 2.0;

  if (this->Dimension == 1)
  {
    this->Sigma = 2.0 / 3.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 10.0 / (7.0 * svtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / svtkMath::Pi();
  }
}

//----------------------------------------------------------------------------
svtkSPHCubicKernel::~svtkSPHCubicKernel() = default;

//----------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void svtkSPHCubicKernel::Initialize(svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* attr)
{
  if (this->Dimension == 1)
  {
    this->Sigma = 2.0 / 3.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 10.0 / (7.0 * svtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / svtkMath::Pi();
  }

  // Sigma must be set before svtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
void svtkSPHCubicKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
