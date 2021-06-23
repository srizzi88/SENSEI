/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricTorus.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParametricTorus.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkParametricTorus);

//----------------------------------------------------------------------------
svtkParametricTorus::svtkParametricTorus()
  : RingRadius(1.0)
  , CrossSectionRadius(0.5)
{
  this->MinimumU = 0;
  this->MaximumU = 2 * svtkMath::Pi();
  this->MinimumV = 0;
  this->MaximumV = 2 * svtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 1;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------
svtkParametricTorus::~svtkParametricTorus() = default;

//----------------------------------------------------------------------------
void svtkParametricTorus::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double* Du = Duvw;
  double* Dv = Duvw + 3;

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double sv = sin(v);
  double t = this->RingRadius + this->CrossSectionRadius * cv;

  // The point
  // Pt[0] = t * cu;
  // Pt[1] = t * su;
  Pt[0] = t * su;
  Pt[1] = t * cu;
  Pt[2] = this->CrossSectionRadius * sv;

  // The derivatives are:
  Du[0] = t * cu;
  Du[1] = -t * su;
  Du[2] = 0;
  Dv[0] = -this->CrossSectionRadius * sv * su;
  Dv[1] = -this->CrossSectionRadius * sv * cu;
  Dv[2] = this->CrossSectionRadius * cv;
}

//----------------------------------------------------------------------------
double svtkParametricTorus::EvaluateScalar(
  double* svtkNotUsed(uv[3]), double* svtkNotUsed(Pt[3]), double* svtkNotUsed(Duv[9]))
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkParametricTorus::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Ring Radius: " << this->RingRadius << "\n";
  os << indent << "Cross-Sectional Radius: " << this->CrossSectionRadius << "\n";
}
