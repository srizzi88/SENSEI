/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricSuperEllipsoid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParametricSuperEllipsoid.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include <cmath>

svtkStandardNewMacro(svtkParametricSuperEllipsoid);

namespace
{
/**
 * Calculate sign(x)*(abs(x)^n).
 */
double SgnPower(double x, double n)
{
  const double eps = 1.0e-06;
  if (x == 0)
  {
    return 0;
  }
  if (n == 0)
  {
    return 1;
  }
  double sgn = (x < 0) ? -1 : 1;
  if (std::abs(x) > eps)
  {
    return sgn * std::pow(std::abs(x), n);
  }
  return 0;
}

} // anonymous namespace

//----------------------------------------------------------------------------
svtkParametricSuperEllipsoid::svtkParametricSuperEllipsoid()
  : XRadius(1)
  , YRadius(1)
  , ZRadius(1)
  , N1(1)
  , N2(1)
{
  this->MinimumU = -svtkMath::Pi();
  this->MaximumU = svtkMath::Pi();
  this->MinimumV = -svtkMath::Pi() / 2.0;
  this->MaximumV = svtkMath::Pi() / 2.0;

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 0;
}

//----------------------------------------------------------------------------
svtkParametricSuperEllipsoid::~svtkParametricSuperEllipsoid() = default;

//----------------------------------------------------------------------------
void svtkParametricSuperEllipsoid::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double* Du = Duvw;
  double* Dv = Duvw + 3;

  for (int i = 0; i < 3; ++i)
  {
    Pt[i] = Du[i] = Dv[i] = 0;
  }

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double sv = sin(v);

  double tmp = SgnPower(cv, this->N1);

  // The point
  Pt[0] = this->XRadius * tmp * SgnPower(su, this->N2);
  Pt[1] = this->YRadius * tmp * SgnPower(cu, this->N2);
  Pt[2] = this->ZRadius * SgnPower(sv, this->N1);
}

//----------------------------------------------------------------------------
double svtkParametricSuperEllipsoid::EvaluateScalar(double*, double*, double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkParametricSuperEllipsoid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "X scale factor: " << this->XRadius << "\n";
  os << indent << "Y scale factor: " << this->YRadius << "\n";
  os << indent << "Z scale factor: " << this->ZRadius << "\n";
  os << indent << "Squareness in the z-axis: " << this->N1 << "\n";
  os << indent << "Squareness in the x-y plane: " << this->N2 << "\n";
}
