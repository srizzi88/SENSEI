/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTimePointUtility.h

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
 * @class   svtkQtTimePointUtility
 * @brief   performs common time operations
 *
 *
 * svtkQtTimePointUtility is provides methods to perform common time operations.
 */

#ifndef svtkQtTimePointUtility_h
#define svtkQtTimePointUtility_h

#include "svtkGUISupportQtSQLModule.h" // For export macro
#include "svtkObject.h"
#include <QDateTime> // Needed for method return types

class SVTKGUISUPPORTQTSQL_EXPORT svtkQtTimePointUtility : public svtkObject
{
public:
  svtkTypeMacro(svtkQtTimePointUtility, svtkObject);

  static QDateTime TimePointToQDateTime(svtkTypeUInt64 time);
  static svtkTypeUInt64 QDateTimeToTimePoint(QDateTime time);
  static svtkTypeUInt64 QDateToTimePoint(QDate date);
  static svtkTypeUInt64 QTimeToTimePoint(QTime time);

protected:
  svtkQtTimePointUtility() {}
  ~svtkQtTimePointUtility() override {}

private:
  svtkQtTimePointUtility(const svtkQtTimePointUtility&) = delete;
  void operator=(const svtkQtTimePointUtility&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtTimePointUtility.h
