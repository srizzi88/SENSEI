/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DummyController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDummyController.h"

#include "ExerciseMultiProcessController.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int DummyController(int argc, char* argv[])
{
  SVTK_CREATE(svtkDummyController, controller);

  controller->Initialize(&argc, &argv, 1);

  int retval = ExerciseMultiProcessController(controller);

  controller->Finalize();

  return retval;
}
