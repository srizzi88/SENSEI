/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointGaussianMapper.h"

#include "svtkObjectFactory.h"
#include "svtkPiecewiseFunction.h"

//-----------------------------------------------------------------------------
svtkAbstractObjectFactoryNewMacro(svtkPointGaussianMapper);

svtkCxxSetObjectMacro(svtkPointGaussianMapper, ScaleFunction, svtkPiecewiseFunction);
svtkCxxSetObjectMacro(svtkPointGaussianMapper, ScalarOpacityFunction, svtkPiecewiseFunction);

//-----------------------------------------------------------------------------
svtkPointGaussianMapper::svtkPointGaussianMapper()
{
  this->ScaleArray = nullptr;
  this->ScaleArrayComponent = 0;
  this->OpacityArray = nullptr;
  this->OpacityArrayComponent = 0;
  this->SplatShaderCode = nullptr;

  this->ScaleFunction = nullptr;
  this->ScaleTableSize = 1024;

  this->ScalarOpacityFunction = nullptr;
  this->OpacityTableSize = 1024;

  this->ScaleFactor = 1.0;
  this->Emissive = 1;
  this->TriangleScale = 3.0;
}

//-----------------------------------------------------------------------------
svtkPointGaussianMapper::~svtkPointGaussianMapper()
{
  this->SetScaleArray(nullptr);
  this->SetOpacityArray(nullptr);
  this->SetSplatShaderCode(nullptr);
  this->SetScalarOpacityFunction(nullptr);
  this->SetScaleFunction(nullptr);
}

//-----------------------------------------------------------------------------
void svtkPointGaussianMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Array: " << (this->ScaleArray ? this->ScaleArray : "(none)") << "\n";
  os << indent << "Scale Array Component: " << this->ScaleArrayComponent << "\n";
  os << indent << "Opacity Array: " << (this->OpacityArray ? this->OpacityArray : "(none)") << "\n";
  os << indent << "Opacity Array Component: " << this->OpacityArrayComponent << "\n";
  os << indent << "SplatShaderCode: " << (this->SplatShaderCode ? this->SplatShaderCode : "(none)")
     << "\n";
  os << indent << "ScaleFactor: " << this->ScaleFactor << "\n";
  os << indent << "Emissive: " << this->Emissive << "\n";
  os << indent << "OpacityTableSize: " << this->OpacityTableSize << "\n";
  os << indent << "ScaleTableSize: " << this->ScaleTableSize << "\n";
  os << indent << "TriangleScale: " << this->TriangleScale << "\n";
}
