/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProgrammableSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMolecule.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkProgrammableSource.h>
#include <svtkRectilinearGrid.h>
#include <svtkStructuredGrid.h>
#include <svtkStructuredPoints.h>
#include <svtkTable.h>
#include <svtkUnstructuredGrid.h>

#define EXECUTE_METHOD(_type)                                                                      \
  void _type##ExecuteMethod(void* args)                                                            \
  {                                                                                                \
    svtkProgrammableSource* self = reinterpret_cast<svtkProgrammableSource*>(args);                  \
    svtk##_type* output = self->Get##_type##Output();                                               \
    if (!output)                                                                                   \
    {                                                                                              \
      std::cerr << "Output type is not of type " #_type "!" << std::endl;                          \
      exit(EXIT_FAILURE);                                                                          \
    }                                                                                              \
  }

EXECUTE_METHOD(PolyData);
EXECUTE_METHOD(StructuredPoints);
EXECUTE_METHOD(StructuredGrid);
EXECUTE_METHOD(UnstructuredGrid);
EXECUTE_METHOD(RectilinearGrid);
EXECUTE_METHOD(Molecule);
EXECUTE_METHOD(Table);

#define TEST_PROGRAMMABLE_SOURCE(_type)                                                            \
  {                                                                                                \
    svtkNew<svtkProgrammableSource> ps;                                                              \
    ps->SetExecuteMethod(&_type##ExecuteMethod, ps.Get());                                         \
    ps->Update();                                                                                  \
    svtk##_type* output = ps->Get##_type##Output();                                                 \
    if (!output)                                                                                   \
    {                                                                                              \
      std::cerr << "Source output type is not of type " #_type "!" << std::endl;                   \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  }

int TestProgrammableSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  TEST_PROGRAMMABLE_SOURCE(PolyData);
  TEST_PROGRAMMABLE_SOURCE(StructuredPoints);
  TEST_PROGRAMMABLE_SOURCE(StructuredGrid);
  TEST_PROGRAMMABLE_SOURCE(UnstructuredGrid);
  TEST_PROGRAMMABLE_SOURCE(RectilinearGrid);
  TEST_PROGRAMMABLE_SOURCE(Molecule);
  TEST_PROGRAMMABLE_SOURCE(Table);
  return EXIT_SUCCESS;
}
