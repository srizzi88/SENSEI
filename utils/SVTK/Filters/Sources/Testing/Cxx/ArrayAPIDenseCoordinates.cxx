/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayAPIDenseCoordinates.cxx

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

int ArrayAPIDenseCoordinates(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    svtkSmartPointer<svtkDiagonalMatrixSource> source =
      svtkSmartPointer<svtkDiagonalMatrixSource>::New();
    source->SetExtents(3);
    source->SetArrayType(svtkDiagonalMatrixSource::DENSE);
    source->SetDiagonal(1.0);
    source->SetSuperDiagonal(0.5);
    source->SetSubDiagonal(-0.5);
    source->Update();

    svtkDenseArray<double>* const array =
      svtkDenseArray<double>::SafeDownCast(source->GetOutput()->GetArray(static_cast<svtkIdType>(0)));

    cout << "dense diagonal matrix:\n";
    svtkPrintMatrixFormat(cout, array);

    cout << "dense diagonal coordinates:\n";
    svtkPrintCoordinateFormat(cout, array);

    test_expression(array);
    test_expression(array->GetValue(svtkArrayCoordinates(0, 0)) == 1.0);
    test_expression(array->GetValue(svtkArrayCoordinates(1, 0)) == -0.5);
    test_expression(array->GetValue(svtkArrayCoordinates(2, 0)) == 0.0);
    test_expression(array->GetValue(svtkArrayCoordinates(0, 1)) == 0.5);
    test_expression(array->GetValue(svtkArrayCoordinates(1, 1)) == 1.0);
    test_expression(array->GetValue(svtkArrayCoordinates(2, 1)) == -0.5);
    test_expression(array->GetValue(svtkArrayCoordinates(0, 2)) == 0.0);
    test_expression(array->GetValue(svtkArrayCoordinates(1, 2)) == 0.5);
    test_expression(array->GetValue(svtkArrayCoordinates(2, 2)) == 1.0);

    for (svtkArray::SizeT n = 0; n != array->GetNonNullSize(); ++n)
    {
      svtkArrayCoordinates coordinates;
      array->GetCoordinatesN(n, coordinates);

      if (coordinates[0] == 0 && coordinates[1] == 0)
      {
        test_expression(array->GetValueN(n) == 1.0);
      }
      else if (coordinates[0] == 0 && coordinates[1] == 1)
      {
        test_expression(array->GetValueN(n) == 0.5);
      }
      else if (coordinates[0] == 1 && coordinates[1] == 0)
      {
        test_expression(array->GetValueN(n) == -0.5);
      }
    }

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
