/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayTransposeMatrix.cxx

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
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>
#include <svtkTransposeMatrix.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArrayTransposeMatrix(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  cout << setprecision(17);

  try
  {
    svtkSmartPointer<svtkSparseArray<double> > source =
      svtkSmartPointer<svtkSparseArray<double> >::New();
    source->Resize(svtkArrayExtents(3, 2));
    source->AddValue(svtkArrayCoordinates(0, 1), 1);
    source->AddValue(svtkArrayCoordinates(1, 0), 2);
    source->AddValue(svtkArrayCoordinates(2, 0), 3);

    cout << "source matrix:\n";
    svtkPrintMatrixFormat(cout, source.GetPointer());

    svtkSmartPointer<svtkArrayData> source_data = svtkSmartPointer<svtkArrayData>::New();
    source_data->AddArray(source);

    svtkSmartPointer<svtkTransposeMatrix> transpose = svtkSmartPointer<svtkTransposeMatrix>::New();
    transpose->SetInputData(source_data);
    transpose->Update();

    svtkSparseArray<double>* const output = svtkSparseArray<double>::SafeDownCast(
      transpose->GetOutput()->GetArray(static_cast<svtkIdType>(0)));
    cout << "output matrix:\n";
    svtkPrintMatrixFormat(cout, output);

    test_expression(output);
    test_expression(output->GetExtent(0).GetSize() == 2);
    test_expression(output->GetExtent(1).GetSize() == 3);

    test_expression(output->GetValue(svtkArrayCoordinates(0, 0)) == 0);
    test_expression(output->GetValue(svtkArrayCoordinates(0, 1)) == 2);
    test_expression(output->GetValue(svtkArrayCoordinates(0, 2)) == 3);
    test_expression(output->GetValue(svtkArrayCoordinates(1, 0)) == 1);
    test_expression(output->GetValue(svtkArrayCoordinates(1, 1)) == 0);
    test_expression(output->GetValue(svtkArrayCoordinates(1, 2)) == 0);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
