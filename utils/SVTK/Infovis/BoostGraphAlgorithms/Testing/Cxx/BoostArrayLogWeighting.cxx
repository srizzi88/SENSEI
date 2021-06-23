/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoostArrayLogWeighting.cxx

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
#include <svtkBoostLogWeighting.h>
#include <svtkDiagonalMatrixSource.h>
#include <svtkSmartPointer.h>
#include <svtkTypedArray.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

static bool close_enough(const double lhs, const double rhs)
{
  return fabs(lhs - rhs) < 1.0e-12;
}

int BoostArrayLogWeighting(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    svtkSmartPointer<svtkDiagonalMatrixSource> source =
      svtkSmartPointer<svtkDiagonalMatrixSource>::New();
    source->SetExtents(3);
    source->SetArrayType(svtkDiagonalMatrixSource::SPARSE);
    source->SetSuperDiagonal(1.0);
    source->SetDiagonal(3.0);
    source->SetSubDiagonal(7.0);

    cout << std::fixed << setprecision(1);
    cout << "sparse diagonal source:\n";
    source->Update();
    svtkPrintMatrixFormat(
      cout, svtkTypedArray<double>::SafeDownCast(source->GetOutput()->GetArray(0)));

    svtkSmartPointer<svtkBoostLogWeighting> log_weighting =
      svtkSmartPointer<svtkBoostLogWeighting>::New();
    log_weighting->AddInputConnection(source->GetOutputPort());

    log_weighting->Update();
    svtkTypedArray<double>* weighted =
      svtkTypedArray<double>::SafeDownCast(log_weighting->GetOutput()->GetArray(0));
    cout << std::fixed << setprecision(17);
    cout << "sparse weighted:\n";
    svtkPrintMatrixFormat(cout, weighted);

    test_expression(weighted);
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(0, 0)), 1.38629436111989057));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(1, 0)), 2.07944154167983575));
    test_expression(close_enough(weighted->GetValue(svtkArrayCoordinates(2, 0)), 0.0));

    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(0, 1)), 0.69314718055994529));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(1, 1)), 1.38629436111989057));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(2, 1)), 2.07944154167983575));

    test_expression(close_enough(weighted->GetValue(svtkArrayCoordinates(0, 2)), 0.0));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(1, 2)), 0.69314718055994529));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(2, 2)), 1.38629436111989057));

    source->SetArrayType(svtkDiagonalMatrixSource::DENSE);
    cout << std::fixed << setprecision(1);
    cout << "dense diagonal source:\n";
    source->Update();
    svtkPrintMatrixFormat(
      cout, svtkTypedArray<double>::SafeDownCast(source->GetOutput()->GetArray(0)));

    log_weighting->Update();
    weighted = svtkTypedArray<double>::SafeDownCast(log_weighting->GetOutput()->GetArray(0));
    cout << std::fixed << setprecision(17);
    cout << "dense weighted:\n";
    svtkPrintMatrixFormat(cout, weighted);

    test_expression(weighted);
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(0, 0)), 1.38629436111989057));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(1, 0)), 2.07944154167983575));
    test_expression(close_enough(weighted->GetValue(svtkArrayCoordinates(2, 0)), 0.0));

    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(0, 1)), 0.69314718055994529));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(1, 1)), 1.38629436111989057));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(2, 1)), 2.07944154167983575));

    test_expression(close_enough(weighted->GetValue(svtkArrayCoordinates(0, 2)), 0.0));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(1, 2)), 0.69314718055994529));
    test_expression(
      close_enough(weighted->GetValue(svtkArrayCoordinates(2, 2)), 1.38629436111989057));

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
