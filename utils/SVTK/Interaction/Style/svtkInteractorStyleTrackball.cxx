/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTrackball.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleTrackball.h"
#include "svtkCommand.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPropPicker.h"
#include "svtkRenderWindowInteractor.h"

svtkStandardNewMacro(svtkInteractorStyleTrackball);

//----------------------------------------------------------------------------
svtkInteractorStyleTrackball::svtkInteractorStyleTrackball()
{
  svtkWarningMacro("svtkInteractorStyleTrackball will be deprecated in"
    << endl
    << "the next release after SVTK 4.0. Please use" << endl
    << "svtkInteractorStyleSwitch instead.");
}

//----------------------------------------------------------------------------
svtkInteractorStyleTrackball::~svtkInteractorStyleTrackball() = default;

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackball::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
