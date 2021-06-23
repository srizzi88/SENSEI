/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProgrammableFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkDirectedGraph.h>
#include <svtkMolecule.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkProgrammableFilter.h>
#include <svtkRectilinearGrid.h>
#include <svtkStructuredGrid.h>
#include <svtkStructuredPoints.h>
#include <svtkTable.h>
#include <svtkUnstructuredGrid.h>

#define EXECUTE_METHOD(_type)                                                                      \
  void _type##ExecuteMethod(void* args)                                                            \
  {                                                                                                \
    svtkProgrammableFilter* self = reinterpret_cast<svtkProgrammableFilter*>(args);                  \
    svtk##_type* input = self->Get##_type##Input();                                                 \
    svtk##_type* output = self->Get##_type##Output();                                               \
    if (!input)                                                                                    \
    {                                                                                              \
      std::cerr << "Input type is not of type " #_type "!" << std::endl;                           \
      exit(EXIT_FAILURE);                                                                          \
    }                                                                                              \
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
EXECUTE_METHOD(Graph);
EXECUTE_METHOD(Molecule);
EXECUTE_METHOD(Table);

#define TEST_PROGRAMMABLE_FILTER_B(_intype, _type)                                                 \
  {                                                                                                \
    svtkNew<svtk##_intype> inData;                                                                   \
    svtkNew<svtkProgrammableFilter> ps;                                                              \
    ps->SetInputData(inData.Get());                                                                \
    ps->SetExecuteMethod(&_type##ExecuteMethod, ps.Get());                                         \
    ps->Update();                                                                                  \
    svtk##_type* output = ps->Get##_type##Output();                                                 \
    if (!output)                                                                                   \
    {                                                                                              \
      std::cerr << "Filter output type is not of type " #_type "!" << std::endl;                   \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  }

#define TEST_PROGRAMMABLE_FILTER_A(_type) TEST_PROGRAMMABLE_FILTER_B(_type, _type)

int TestProgrammableFilter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  TEST_PROGRAMMABLE_FILTER_A(PolyData);
  TEST_PROGRAMMABLE_FILTER_A(StructuredPoints);
  TEST_PROGRAMMABLE_FILTER_A(StructuredGrid);
  TEST_PROGRAMMABLE_FILTER_A(UnstructuredGrid);
  TEST_PROGRAMMABLE_FILTER_A(RectilinearGrid);
  TEST_PROGRAMMABLE_FILTER_B(DirectedGraph, Graph);
  TEST_PROGRAMMABLE_FILTER_A(Molecule);
  TEST_PROGRAMMABLE_FILTER_A(Table);
  return EXIT_SUCCESS;
}
