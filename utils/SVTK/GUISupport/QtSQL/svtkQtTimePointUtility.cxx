/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTimePointUtility.cxx

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

#include "svtkQtTimePointUtility.h"

#include "svtkObjectFactory.h"

QDateTime svtkQtTimePointUtility::TimePointToQDateTime(svtkTypeUInt64 time)
{
  int julianDay = time / 86400000;
  QDate qdate = QDate::fromJulianDay(julianDay);
  int hour = static_cast<int>(time % 86400000) / 3600000;
  int minute = static_cast<int>(time % 3600000) / 60000;
  int second = static_cast<int>(time % 60000) / 1000;
  int millis = static_cast<int>(time % 1000);
  QTime qtime(hour, minute, second, millis);
  QDateTime dt(qdate, qtime);
  return dt;
}

svtkTypeUInt64 svtkQtTimePointUtility::QDateTimeToTimePoint(QDateTime time)
{
  svtkTypeUInt64 timePoint = QDateToTimePoint(time.date()) + QTimeToTimePoint(time.time());
  return timePoint;
}

svtkTypeUInt64 svtkQtTimePointUtility::QDateToTimePoint(QDate date)
{
  svtkTypeUInt64 timePoint = static_cast<svtkTypeUInt64>(date.toJulianDay()) * 86400000;
  return timePoint;
}

svtkTypeUInt64 svtkQtTimePointUtility::QTimeToTimePoint(QTime time)
{
  svtkTypeUInt64 timePoint =
    +time.hour() * 3600000 + time.minute() * 60000 + time.second() * 1000 + time.msec();
  return timePoint;
}
