/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRIBLight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRIBLight.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkRIBLight);

svtkRIBLight::svtkRIBLight()
{
  this->Shadows = 0;
  // create a svtkLight that can be rendered
  this->Light = svtkLight::New();
}

svtkRIBLight::~svtkRIBLight()
{
  if (this->Light)
  {
    this->Light->Delete();
  }
}

void svtkRIBLight::Render(svtkRenderer* ren, int index)
{
  int ref;

  // Copy this light's ivars into the light to be rendered
  ref = this->Light->GetReferenceCount();
  this->Light->DeepCopy(this);
  // this->Light->SetDeleteMethod(nullptr);
  this->Light->SetReferenceCount(ref);

  // Render the light
  this->Light->Render(ren, index);
}

void svtkRIBLight::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Shadows: " << (this->Shadows ? "On\n" : "Off\n");
}
