/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayDiagonalMatrixSource.cxx

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
#include <svtkDenseArray.h>
#include <svtkDiagonalMatrixSource.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArrayDiagonalMatrixSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    svtkSmartPointer<svtkDiagonalMatrixSource> source =
      svtkSmartPointer<svtkDiagonalMatrixSource>::New();
    source->SetExtents(3);
    source->SetArrayType(svtkDiagonalMatrixSource::SPARSE);
    source->SetDiagonal(1.0);
    source->SetSuperDiagonal(0.5);
    source->SetSubDiagonal(-0.5);
    source->Update();

    svtkSparseArray<double>* const sparse_array = svtkSparseArray<double>::SafeDownCast(
      source->GetOutput()->GetArray(static_cast<svtkIdType>(0)));

    cout << "sparse diagonal matrix:\n";
    svtkPrintMatrixFormat(cout, sparse_array);

    test_expression(sparse_array);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 0)) == 1.0);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(1, 0)) == -0.5);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(2, 0)) == 0.0);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 1)) == 0.5);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(1, 1)) == 1.0);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(2, 1)) == -0.5);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(0, 2)) == 0.0);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(1, 2)) == 0.5);
    test_expression(sparse_array->GetValue(svtkArrayCoordinates(2, 2)) == 1.0);

    source->SetArrayType(svtkDiagonalMatrixSource::DENSE);
    source->Update();

    svtkDenseArray<double>* const dense_array =
      svtkDenseArray<double>::SafeDownCast(source->GetOutput()->GetArray(static_cast<svtkIdType>(0)));

    cout << "dense diagonal matrix:\n";
    svtkPrintMatrixFormat(cout, dense_array);

    test_expression(dense_array);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(0, 0)) == 1.0);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(1, 0)) == -0.5);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(2, 0)) == 0.0);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(0, 1)) == 0.5);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(1, 1)) == 1.0);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(2, 1)) == -0.5);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(0, 2)) == 0.0);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(1, 2)) == 0.5);
    test_expression(dense_array->GetValue(svtkArrayCoordinates(2, 2)) == 1.0);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
