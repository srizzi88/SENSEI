/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTDxInteractorStyleSettings.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTDxInteractorStyleSettings.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkTDxInteractorStyleSettings);

// ----------------------------------------------------------------------------
svtkTDxInteractorStyleSettings::svtkTDxInteractorStyleSettings()
{
  this->AngleSensitivity = 1.0;
  this->UseRotationX = true;
  this->UseRotationY = true;
  this->UseRotationZ = true;
  this->TranslationXSensitivity = 1.0;
  this->TranslationYSensitivity = 1.0;
  this->TranslationZSensitivity = 1.0;
}

// ----------------------------------------------------------------------------
svtkTDxInteractorStyleSettings::~svtkTDxInteractorStyleSettings() = default;

//----------------------------------------------------------------------------
void svtkTDxInteractorStyleSettings::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AngleSensitivity: " << this->AngleSensitivity << endl;
  os << indent << "UseRotationX: " << this->UseRotationX << endl;
  os << indent << "UseRotationY: " << this->UseRotationY << endl;
  os << indent << "UseRotationZ: " << this->UseRotationZ << endl;

  os << indent << "TranslationXSensitivity: " << this->TranslationXSensitivity << endl;
  os << indent << "TranslationYSensitivity: " << this->TranslationYSensitivity << endl;
  os << indent << "TranslationZSensitivity: " << this->TranslationZSensitivity << endl;
}
