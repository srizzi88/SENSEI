/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThresholdTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkArrayPrint.h"
#include "svtkDenseArray.h"
#include "svtkDoubleArray.h"
#include "svtkIntArray.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTableToArray.h"

#include "svtkSmartPointer.h"

#include <stdexcept>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int TestTableToArray(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    SVTK_CREATE(svtkTable, table);

    SVTK_CREATE(svtkIntArray, int_array);
    int_array->SetName("A");
    int_array->InsertNextValue(1);
    int_array->InsertNextValue(2);
    int_array->InsertNextValue(3);
    int_array->InsertNextValue(4);
    table->AddColumn(int_array);

    SVTK_CREATE(svtkDoubleArray, double_array);
    double_array->SetName("B");
    double_array->InsertNextValue(1.1);
    double_array->InsertNextValue(1.2);
    double_array->InsertNextValue(1.3);
    double_array->InsertNextValue(1.4);
    table->AddColumn(double_array);

    SVTK_CREATE(svtkStringArray, string_array);
    string_array->SetName("C");
    string_array->InsertNextValue("11");
    string_array->InsertNextValue("12");
    string_array->InsertNextValue("13");
    string_array->InsertNextValue("14");
    table->AddColumn(string_array);

    SVTK_CREATE(svtkTableToArray, table_to_array);
    table_to_array->SetInputData(0, table);
    table_to_array->AddColumn("C");
    table_to_array->AddColumn(1);
    table_to_array->AddColumn(static_cast<svtkIdType>(0));
    table_to_array->AddAllColumns();

    table_to_array->Update();
    test_expression(table_to_array->GetOutput());
    test_expression(1 == table_to_array->GetOutput()->GetNumberOfArrays());
    svtkDenseArray<double>* const array =
      svtkDenseArray<double>::SafeDownCast(table_to_array->GetOutput()->GetArray(0));
    test_expression(array);
    test_expression(2 == array->GetDimensions());
    test_expression(4 == array->GetExtent(0).GetSize());
    test_expression(6 == array->GetExtent(1).GetSize());
    test_expression(11 == array->GetValue(0, 0));
    test_expression(1.1 == array->GetValue(0, 1));
    test_expression(1 == array->GetValue(0, 2));
    test_expression(1 == array->GetValue(0, 3));
    test_expression(1.1 == array->GetValue(0, 4));
    test_expression(11 == array->GetValue(0, 5));
    test_expression(14 == array->GetValue(3, 0));

    svtkPrintMatrixFormat(std::cout, array);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
