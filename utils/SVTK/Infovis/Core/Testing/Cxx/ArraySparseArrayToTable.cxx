/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArraySparseArrayToTable.cxx

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

#include <svtkAbstractArray.h>
#include <svtkArrayData.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>
#include <svtkSparseArrayToTable.h>
#include <svtkTable.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArraySparseArrayToTable(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    svtkSmartPointer<svtkSparseArray<double> > array =
      svtkSmartPointer<svtkSparseArray<double> >::New();
    array->Resize(10, 10, 10);
    array->SetDimensionLabel(0, "i");
    array->SetDimensionLabel(1, "j");
    array->SetDimensionLabel(2, "k");
    array->AddValue(0, 0, 0, 1);
    array->AddValue(1, 2, 3, 2);
    array->AddValue(4, 5, 6, 3);

    svtkSmartPointer<svtkArrayData> array_data = svtkSmartPointer<svtkArrayData>::New();
    array_data->AddArray(array);

    svtkSmartPointer<svtkSparseArrayToTable> convert = svtkSmartPointer<svtkSparseArrayToTable>::New();
    convert->SetInputData(0, array_data);
    convert->SetValueColumn("value");
    convert->Update();

    svtkTable* const table = convert->GetOutput();
    table->Dump(8);

    test_expression(table->GetNumberOfColumns() == 4);
    test_expression(table->GetColumn(0)->GetName() == svtkStdString("i"));
    test_expression(table->GetColumn(1)->GetName() == svtkStdString("j"));
    test_expression(table->GetColumn(2)->GetName() == svtkStdString("k"));
    test_expression(table->GetColumn(3)->GetName() == svtkStdString("value"));

    test_expression(table->GetNumberOfRows() == 3);

    test_expression(table->GetValue(0, 0).ToInt() == 0);
    test_expression(table->GetValue(0, 1).ToInt() == 0);
    test_expression(table->GetValue(0, 2).ToInt() == 0);
    test_expression(table->GetValue(0, 3).ToDouble() == 1);
    test_expression(table->GetValue(1, 0).ToInt() == 1);
    test_expression(table->GetValue(1, 1).ToInt() == 2);
    test_expression(table->GetValue(1, 2).ToInt() == 3);
    test_expression(table->GetValue(1, 3).ToDouble() == 2);
    test_expression(table->GetValue(2, 0).ToInt() == 4);
    test_expression(table->GetValue(2, 1).ToInt() == 5);
    test_expression(table->GetValue(2, 2).ToInt() == 6);
    test_expression(table->GetValue(2, 3).ToDouble() == 3);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
