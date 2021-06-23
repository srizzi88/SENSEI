/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayNormalizeMatrixVectors.cxx

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
#include <svtkDiagonalMatrixSource.h>
#include <svtkNormalizeMatrixVectors.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>

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

int ArrayNormalizeMatrixVectors(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
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

    std::cout << std::fixed << setprecision(1);
    std::cout << "sparse diagonal source:\n";
    source->Update();
    svtkPrintMatrixFormat(std::cout,
      svtkTypedArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<svtkIdType>(0))));

    svtkSmartPointer<svtkNormalizeMatrixVectors> normalize =
      svtkSmartPointer<svtkNormalizeMatrixVectors>::New();
    normalize->AddInputConnection(source->GetOutputPort());
    normalize->SetVectorDimension(1);

    normalize->Update();
    svtkTypedArray<double>* normalized = svtkTypedArray<double>::SafeDownCast(
      normalize->GetOutput()->GetArray(static_cast<svtkIdType>(0)));
    std::cout << std::fixed << setprecision(17);
    std::cout << "sparse normalized:\n";
    svtkPrintMatrixFormat(std::cout, normalized);

    test_expression(normalized);
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(0, 0)), 0.89442719099991586));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(1, 0)), -0.44721359549995793));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(2, 0)), 0.00000000000000000));

    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(0, 1)), 0.40824829046386307));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(1, 1)), 0.81649658092772615));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(2, 1)), -0.40824829046386307));

    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(0, 2)), 0.00000000000000000));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(1, 2)), 0.44721359549995793));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(2, 2)), 0.89442719099991586));

    source->SetArrayType(svtkDiagonalMatrixSource::DENSE);
    std::cout << std::fixed << setprecision(1);
    std::cout << "dense diagonal source:\n";
    source->Update();
    svtkPrintMatrixFormat(std::cout,
      svtkTypedArray<double>::SafeDownCast(
        source->GetOutput()->GetArray(static_cast<svtkIdType>(0))));

    normalize->Update();
    normalized = svtkTypedArray<double>::SafeDownCast(
      normalize->GetOutput()->GetArray(static_cast<svtkIdType>(0)));
    std::cout << std::fixed << setprecision(17);
    std::cout << "dense normalized:\n";
    svtkPrintMatrixFormat(std::cout, normalized);

    test_expression(normalized);
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(0, 0)), 0.89442719099991586));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(1, 0)), -0.44721359549995793));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(2, 0)), 0.00000000000000000));

    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(0, 1)), 0.40824829046386307));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(1, 1)), 0.81649658092772615));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(2, 1)), -0.40824829046386307));

    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(0, 2)), 0.00000000000000000));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(1, 2)), 0.44721359549995793));
    test_expression(
      close_enough(normalized->GetValue(svtkArrayCoordinates(2, 2)), 0.89442719099991586));

    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
