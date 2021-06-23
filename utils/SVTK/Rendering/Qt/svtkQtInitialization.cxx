/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtInitialization.cxx

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

#include "svtkQtInitialization.h"
#include "svtkObjectFactory.h"

#include <QApplication>

svtkStandardNewMacro(svtkQtInitialization);

svtkQtInitialization::svtkQtInitialization()
{
  this->Application = nullptr;
  if (!QApplication::instance())
  {
    int argc = 0;
    this->Application = new QApplication(argc, nullptr);
  }
}

svtkQtInitialization::~svtkQtInitialization()
{
  delete this->Application;
}

void svtkQtInitialization::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "QApplication: " << QApplication::instance() << endl;
}
