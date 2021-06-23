/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricEnneper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParametricEnneper.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkParametricEnneper);

//----------------------------------------------------------------------------
svtkParametricEnneper::svtkParametricEnneper()
{
  // Preset triangulation parameters
  this->MinimumU = -2.0;
  this->MaximumU = 2.0;
  this->MinimumV = -2.0;
  this->MaximumV = 2.0;

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------
svtkParametricEnneper::~svtkParametricEnneper() = default;

//----------------------------------------------------------------------------
void svtkParametricEnneper::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{

  double u = uvw[0];
  double v = uvw[1];
  double* Du = Duvw;
  double* Dv = Duvw + 3;

  // The point
  Pt[0] = u - u * u * u / 3 + u * v * v;
  Pt[1] = v - v * v * v / 3 + u * u * v;
  Pt[2] = u * u - v * v;

  // The derivatives are:
  Du[0] = 1 - u * u + v * v;
  Dv[0] = 2 * u * v;
  Du[1] = 2 * u * v;
  Dv[1] = 1 - v * v + u * u;
  Du[2] = 2 * u;
  Dv[2] = -2 * v;
}

//----------------------------------------------------------------------------
double svtkParametricEnneper::EvaluateScalar(double*, double*, double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkParametricEnneper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
