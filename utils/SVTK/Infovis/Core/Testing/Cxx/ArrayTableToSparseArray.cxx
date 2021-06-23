/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayTableToSparseArray.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkArrayData.h>
#include <svtkArrayPrint.h>
#include <svtkDoubleArray.h>
#include <svtkIdTypeArray.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>
#include <svtkTable.h>
#include <svtkTableToSparseArray.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArrayTableToSparseArray(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    // Generate an input table ...
    svtkSmartPointer<svtkIdTypeArray> i = svtkSmartPointer<svtkIdTypeArray>::New();
    i->SetName("i");

    svtkSmartPointer<svtkIdTypeArray> j = svtkSmartPointer<svtkIdTypeArray>::New();
    j->SetName("j");

    svtkSmartPointer<svtkIdTypeArray> k = svtkSmartPointer<svtkIdTypeArray>::New();
    k->SetName("k");

    svtkSmartPointer<svtkDoubleArray> value = svtkSmartPointer<svtkDoubleArray>::New();
    value->SetName("value");

    svtkSmartPointer<svtkTable> table = svtkSmartPointer<svtkTable>::New();
    table->AddColumn(i);
    table->AddColumn(j);
    table->AddColumn(k);
    table->AddColumn(value);

    table->InsertNextBlankRow();
    table->SetValue(0, 0, 0);
    table->SetValue(0, 1, 0);
    table->SetValue(0, 2, 0);
    table->SetValue(0, 3, 1);

    table->InsertNextBlankRow();
    table->SetValue(1, 0, 1);
    table->SetValue(1, 1, 2);
    table->SetValue(1, 2, 3);
    table->SetValue(1, 3, 2);

    table->InsertNextBlankRow();
    table->SetValue(2, 0, 4);
    table->SetValue(2, 1, 5);
    table->SetValue(2, 2, 6);
    table->SetValue(2, 3, 3);

    // Run it through svtkTableToSparseArray ...
    svtkSmartPointer<svtkTableToSparseArray> source = svtkSmartPointer<svtkTableToSparseArray>::New();
    source->SetInputData(table);
    source->AddCoordinateColumn("i");
    source->AddCoordinateColumn("j");
    source->AddCoordinateColumn("k");
    source->SetValueColumn("value");
    source->Update();
    {
      svtkSparseArray<double>* const sparse_array = svtkSparseArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<svtkIdType>(0)));
      test_expression(sparse_array);

      sparse_array->Print(std::cerr);

      test_expression(sparse_array->GetExtent(0) == svtkArrayRange(0, 5));
      test_expression(sparse_array->GetExtent(1) == svtkArrayRange(0, 6));
      test_expression(sparse_array->GetExtent(2) == svtkArrayRange(0, 7));

      test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 0, 0)) == 1);
      test_expression(sparse_array->GetValue(svtkArrayCoordinates(1, 2, 3)) == 2);
      test_expression(sparse_array->GetValue(svtkArrayCoordinates(4, 5, 6)) == 3);

      test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 0, 1)) == 0);
    }

    // Change svtkTableToSparseArray to use explicit output extents ...
    source->SetOutputExtents(svtkArrayExtents(11, 12, 13));
    source->Update();
    {
      svtkSparseArray<double>* const sparse_array = svtkSparseArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<svtkIdType>(0)));
      test_expression(sparse_array);

      sparse_array->Print(std::cerr);

      test_expression(sparse_array->GetExtent(0) == svtkArrayRange(0, 11));
      test_expression(sparse_array->GetExtent(1) == svtkArrayRange(0, 12));
      test_expression(sparse_array->GetExtent(2) == svtkArrayRange(0, 13));

      test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 0, 0)) == 1);
      test_expression(sparse_array->GetValue(svtkArrayCoordinates(1, 2, 3)) == 2);
      test_expression(sparse_array->GetValue(svtkArrayCoordinates(4, 5, 6)) == 3);

      test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 0, 1)) == 0);
    }

    return 0;
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << endl;
    return 1;
  }
}
