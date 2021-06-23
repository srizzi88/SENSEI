/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVtkQtTableView.cxx

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

#include "svtkQtTableView.h"

#include "svtkDataObjectToTable.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTable.h"

#include <QApplication>
#include <QTimer>
#include <QWidget>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestVtkQtTableView(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Create a sphere and create a svtkTable from its point data (normal vectors)
  SVTK_CREATE(svtkSphereSource, sphereSource);
  SVTK_CREATE(svtkDataObjectToTable, tableConverter);
  tableConverter->SetInputConnection(sphereSource->GetOutputPort());
  tableConverter->SetFieldType(svtkDataObjectToTable::POINT_DATA);
  tableConverter->Update();
  svtkTable* pointTable = tableConverter->GetOutput();

  // Show the table in a svtkQtTableView with split columns on
  SVTK_CREATE(svtkQtTableView, tableView);
  tableView->SetSplitMultiComponentColumns(true);
  tableView->AddRepresentationFromInput(pointTable);
  tableView->Update();
  tableView->GetWidget()->show();

  // Start the Qt event loop to run the application
  QTimer::singleShot(500, &app, SLOT(quit()));
  return app.exec();
}
