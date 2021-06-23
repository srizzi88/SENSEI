/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtInitialization.h

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
/**
 * @class   svtkQtInitialization
 * @brief   Initializes a Qt application.
 *
 *
 * Utility class that initializes Qt by creating an instance of
 * QApplication in its ctor, if one doesn't already exist.
 * This is mainly of use in ParaView with filters that use Qt in
 * their implementation - create an instance of svtkQtInitialization
 * prior to instantiating any filters that require Qt.
 */

#ifndef svtkQtInitialization_h
#define svtkQtInitialization_h

#include "svtkObject.h"
#include "svtkRenderingQtModule.h" // For export macro

class QApplication;

class SVTKRENDERINGQT_EXPORT svtkQtInitialization : public svtkObject
{
public:
  static svtkQtInitialization* New();
  svtkTypeMacro(svtkQtInitialization, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkQtInitialization();
  ~svtkQtInitialization() override;

private:
  svtkQtInitialization(const svtkQtInitialization&) = delete;
  void operator=(const svtkQtInitialization&) = delete;

  QApplication* Application;
};

#endif // svtkQtInitialization_h
