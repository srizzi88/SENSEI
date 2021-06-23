/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTestingObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTestingObjectFactory.h"
#include "svtkTestingInteractor.h"
#include "svtkVersion.h"

svtkStandardNewMacro(svtkTestingObjectFactory);

SVTK_CREATE_CREATE_FUNCTION(svtkTestingInteractor);

svtkTestingObjectFactory::svtkTestingObjectFactory()
{
  this->RegisterOverride("svtkRenderWindowInteractor", "svtkTestingInteractor",
    "Overrides for testing", 1, svtkObjectFactoryCreatesvtkTestingInteractor);
}

const char* svtkTestingObjectFactory::GetSVTKSourceVersion()
{
  return SVTK_SOURCE_VERSION;
}

void svtkTestingObjectFactory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Description: " << this->GetDescription() << endl;
}
