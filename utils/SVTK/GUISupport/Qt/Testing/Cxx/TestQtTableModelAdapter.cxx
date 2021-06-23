/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTableModelAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// Tests svtkQtTableModelAdapter.

#include "svtkDoubleArray.h"
#include "svtkIntArray.h"
#include "svtkQtTableModelAdapter.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestQtTableModelAdapter(int, char*[])
{
  int numRows = 10;
  int errors = 0;
  SVTK_CREATE(svtkTable, table);
  SVTK_CREATE(svtkIntArray, intArr);
  intArr->SetName("int");
  SVTK_CREATE(svtkDoubleArray, doubleArr);
  doubleArr->SetName("double");
  for (int i = 0; i < numRows; ++i)
  {
    intArr->InsertNextValue(i);
    doubleArr->InsertNextValue(-i);
  }
  table->AddColumn(intArr);
  table->AddColumn(doubleArr);
  svtkQtTableModelAdapter adapter(table);
  if (adapter.rowCount(QModelIndex()) != numRows)
  {
    cerr << "ERROR: Wrong number of rows." << endl;
    ++errors;
  }
  if (adapter.columnCount(QModelIndex()) != 2)
  {
    cerr << "ERROR: Wrong number of columns." << endl;
    ++errors;
  }
  for (int i = 0; i < numRows; ++i)
  {
    QModelIndex ind = adapter.index(i, 0);
#if 0 // FIXME to work with new selection conversion routines
    QModelIndex pind = adapter.PedigreeToQModelIndex(i);
    if (ind != pind)
    {
      cerr << "ERROR: Pedigree lookup failed." << endl;
      ++errors;
    }
#endif
    if (adapter.rowCount(ind) != 0)
    {
      cerr << "ERROR: Row should have zero sub-rows." << endl;
      ++errors;
    }
    if (adapter.parent(ind) != QModelIndex())
    {
      cerr << "ERROR: Wrong parent." << endl;
      ++errors;
    }
  }
  return errors;
}
