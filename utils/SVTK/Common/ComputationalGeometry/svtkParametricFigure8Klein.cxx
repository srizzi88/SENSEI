/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricFigure8Klein.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParametricFigure8Klein.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkParametricFigure8Klein);

//----------------------------------------------------------------------------
svtkParametricFigure8Klein::svtkParametricFigure8Klein()
{
  // Preset triangulation parameters
  this->MinimumU = -svtkMath::Pi();
  this->MinimumV = -svtkMath::Pi();
  this->MaximumU = svtkMath::Pi();
  this->MaximumV = svtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 1;
  this->TwistU = 1;
  this->TwistV = 0;
  this->ClockwiseOrdering = 0;
  this->DerivativesAvailable = 1;
  this->Radius = 1;
}

//----------------------------------------------------------------------------
svtkParametricFigure8Klein::~svtkParametricFigure8Klein() = default;

//----------------------------------------------------------------------------
void svtkParametricFigure8Klein::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double* Du = Duvw;
  double* Dv = Duvw + 3;

  double cu = cos(u);
  double cu2 = cos(u / 2);
  double su = sin(u);
  double su2 = sin(u / 2);
  double cv = cos(v);
  double c2v = cos(2 * v);
  double s2v = sin(2 * v);
  double sv = sin(v);
  double t = this->Radius + sv * cu2 - s2v * su2 / 2;

  // The point
  Pt[0] = cu * t;
  Pt[1] = su * t;
  Pt[2] = su2 * sv + cu2 * s2v / 2;

  // The derivatives are:
  Du[0] = -Pt[1] - cu * (2 * sv * su2 + s2v * cu2) / 4;
  Du[1] = Pt[0] - su * (2 * sv * su2 + s2v * cu2) / 4;
  Du[2] = cu2 * sv / 2 - su2 * s2v / 4;
  Dv[0] = cu * (cv * cu2 - c2v * su2);
  Dv[1] = su * (cv * cu2 - c2v * su2);
  Dv[2] = su2 * cv / 2 + cu2 * c2v;
}

//----------------------------------------------------------------------------
double svtkParametricFigure8Klein::EvaluateScalar(double*, double*, double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkParametricFigure8Klein::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
}
