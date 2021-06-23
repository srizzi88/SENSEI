/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayExtractFactoredArray.cxx

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
#include <svtkExtractArray.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArrayExtractFactoredArray(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    svtkSmartPointer<svtkSparseArray<double> > a = svtkSmartPointer<svtkSparseArray<double> >::New();
    svtkSmartPointer<svtkSparseArray<double> > b = svtkSmartPointer<svtkSparseArray<double> >::New();

    svtkSmartPointer<svtkArrayData> factored = svtkSmartPointer<svtkArrayData>::New();
    factored->AddArray(a);
    factored->AddArray(b);

    svtkSmartPointer<svtkExtractArray> extract = svtkSmartPointer<svtkExtractArray>::New();
    extract->SetInputData(factored);

    extract->SetIndex(0);
    extract->Update();
    test_expression(extract->GetOutput()->GetArray(static_cast<svtkIdType>(0)) == a.GetPointer());

    extract->SetIndex(1);
    extract->Update();
    test_expression(extract->GetOutput()->GetArray(static_cast<svtkIdType>(0)) == b.GetPointer());

    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
