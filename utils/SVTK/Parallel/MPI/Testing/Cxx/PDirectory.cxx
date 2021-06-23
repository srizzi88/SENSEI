/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PDirectory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtk_mpi.h>

#include "svtkMPIController.h"
#include "svtkPDirectory.h"
#include "svtkPSystemTools.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int PDirectory(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  SVTK_CREATE(svtkMPIController, controller);

  controller->Initialize(&argc, &argv, 1);
  controller->SetGlobalController(controller);

  int retVal = 0; // success

  std::string str = svtkPSystemTools::GetCurrentWorkingDirectory();

  SVTK_CREATE(svtkPDirectory, directory);

  if (!directory->Load(str))
  {
    svtkGenericWarningMacro("Could not load directory");
    retVal++;
  }

  if (directory->GetNumberOfFiles() < 3)
  {
    svtkGenericWarningMacro("Missing files");
    retVal++;
  }

  bool hasFile = false;
  for (svtkIdType i = 0; i < directory->GetNumberOfFiles(); i++)
  {
    hasFile = hasFile || (strcmp(directory->GetFile(i), "cmake_install.cmake") == 0);
  }
  if (!hasFile)
  {
    svtkGenericWarningMacro("Missing cmake_install.cmake");
    retVal++;
  }

  controller->SetGlobalController(nullptr);
  controller->Finalize();

  return retVal;
}
