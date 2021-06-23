/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayToTable.cxx

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
#include <svtkArrayToTable.h>
#include <svtkDenseArray.h>
#include <svtkSmartPointer.h>
#include <svtkSparseArray.h>
#include <svtkTable.h>

#include <iostream>
#include <stdexcept>

#define test_expression(expression)                                                                \
  {                                                                                                \
    if (!(expression))                                                                             \
      throw std::runtime_error("Expression failed: " #expression);                                 \
  }

int ArrayToTable(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  try
  {
    svtkSmartPointer<svtkDenseArray<svtkStdString> > a =
      svtkSmartPointer<svtkDenseArray<svtkStdString> >::New();
    a->Resize(2);
    a->SetValue(0, "Howdy");
    a->SetValue(1, "World!");

    svtkSmartPointer<svtkArrayData> b = svtkSmartPointer<svtkArrayData>::New();
    b->AddArray(a);

    svtkSmartPointer<svtkArrayToTable> c = svtkSmartPointer<svtkArrayToTable>::New();
    c->SetInputData(0, b);
    c->Update();

    test_expression(c->GetOutput()->GetNumberOfColumns() == 1);
    test_expression(c->GetOutput()->GetNumberOfRows() == 2);
    test_expression(svtkStdString(c->GetOutput()->GetColumn(0)->GetName()).empty());
    test_expression(c->GetOutput()->GetValue(0, 0).ToString() == "Howdy");
    test_expression(c->GetOutput()->GetValue(1, 0).ToString() == "World!");

    svtkSmartPointer<svtkSparseArray<double> > d = svtkSmartPointer<svtkSparseArray<double> >::New();
    d->Resize(2, 2);
    d->SetValue(0, 0, 1.0);
    d->SetValue(1, 1, 2.0);

    svtkSmartPointer<svtkArrayData> e = svtkSmartPointer<svtkArrayData>::New();
    e->AddArray(d);

    svtkSmartPointer<svtkArrayToTable> f = svtkSmartPointer<svtkArrayToTable>::New();
    f->SetInputData(0, e);
    f->Update();

    test_expression(f->GetOutput()->GetNumberOfColumns() == 2);
    test_expression(f->GetOutput()->GetNumberOfRows() == 2);
    test_expression(svtkStdString(f->GetOutput()->GetColumn(0)->GetName()) == "0");
    test_expression(svtkStdString(f->GetOutput()->GetColumn(1)->GetName()) == "1");
    test_expression(f->GetOutput()->GetValue(0, 0).ToDouble() == 1.0);
    test_expression(f->GetOutput()->GetValue(0, 1).ToDouble() == 0.0);
    test_expression(f->GetOutput()->GetValue(1, 0).ToDouble() == 0.0);
    test_expression(f->GetOutput()->GetValue(1, 1).ToDouble() == 2.0);

    return 0;
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
}
