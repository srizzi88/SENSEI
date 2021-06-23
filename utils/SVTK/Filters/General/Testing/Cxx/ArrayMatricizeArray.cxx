/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayMatricizeArray.cxx

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
#include <svtkMatricizeArray.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArrayMatricizeArray(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    // Create an array ...
    svtkSmartPointer<svtkSparseArray<double> > array =
      svtkSmartPointer<svtkSparseArray<double> >::New();
    array->Resize(svtkArrayExtents(2, 2, 2));

    double value = 0;
    const svtkArrayExtents extents = array->GetExtents();
    for (int i = extents[0].GetBegin(); i != extents[0].GetEnd(); ++i)
    {
      for (int j = extents[1].GetBegin(); j != extents[1].GetEnd(); ++j)
      {
        for (int k = extents[2].GetBegin(); k != extents[2].GetEnd(); ++k)
        {
          array->AddValue(svtkArrayCoordinates(i, j, k), value++);
        }
      }
    }

    std::cout << "array source:\n";
    svtkPrintCoordinateFormat(std::cout, array.GetPointer());

    // Create an array data object to hold it ...
    svtkSmartPointer<svtkArrayData> array_data = svtkSmartPointer<svtkArrayData>::New();
    array_data->AddArray(array);

    // Matricize it ...
    svtkSmartPointer<svtkMatricizeArray> matricize = svtkSmartPointer<svtkMatricizeArray>::New();
    matricize->SetInputData(array_data);
    matricize->SetSliceDimension(0);
    matricize->Update();

    svtkSparseArray<double>* const matricized_array = svtkSparseArray<double>::SafeDownCast(
      matricize->GetOutput()->GetArray(static_cast<svtkIdType>(0)));
    test_expression(matricized_array);

    std::cout << "matricize output:\n";
    svtkPrintCoordinateFormat(std::cout, matricized_array);

    test_expression(matricized_array->GetValue(svtkArrayCoordinates(0, 0)) == 0);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(0, 1)) == 1);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(0, 2)) == 2);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(0, 3)) == 3);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(1, 0)) == 4);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(1, 1)) == 5);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(1, 2)) == 6);
    test_expression(matricized_array->GetValue(svtkArrayCoordinates(1, 3)) == 7);

    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
