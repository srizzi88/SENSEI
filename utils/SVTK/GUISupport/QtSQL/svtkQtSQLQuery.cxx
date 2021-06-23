/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtSQLQuery.cxx

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

// Check for Qt SQL module before defining this class.
#include <qglobal.h>
#if (QT_EDITION & QT_MODULE_SQL)

#include "svtkQtSQLQuery.h"

#include "svtkCharArray.h"
#include "svtkObjectFactory.h"
#include "svtkQtSQLDatabase.h"
#include "svtkQtTimePointUtility.h"
#include "svtkVariantArray.h"

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QtSql/QSqlQuery>
#include <QtSql/QtSql>
#include <string>
#include <vector>

class svtkQtSQLQueryInternals
{
public:
  QSqlQuery QtQuery;
  std::vector<std::string> FieldNames;
};

svtkStandardNewMacro(svtkQtSQLQuery);

svtkQtSQLQuery::svtkQtSQLQuery()
{
  this->Internals = new svtkQtSQLQueryInternals();
  this->Internals->QtQuery.setForwardOnly(true);
  this->LastErrorText = nullptr;
}

svtkQtSQLQuery::~svtkQtSQLQuery()
{
  delete this->Internals;
  this->SetLastErrorText(nullptr);
}

void svtkQtSQLQuery::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastErrorText: " << (this->LastErrorText ? this->LastErrorText : "nullptr")
     << endl;
}

bool svtkQtSQLQuery::HasError()
{
  return this->Internals->QtQuery.lastError().isValid();
}

const char* svtkQtSQLQuery::GetLastErrorText()
{
  this->SetLastErrorText(this->Internals->QtQuery.lastError().text().toLatin1());
  return this->LastErrorText;
}

bool svtkQtSQLQuery::Execute()
{
  if (this->Query == nullptr)
  {
    svtkErrorMacro("Query string must be non-null.");
    return false;
  }
  this->Internals->QtQuery =
    svtkQtSQLDatabase::SafeDownCast(this->Database)->QtDatabase.exec(this->Query);

  QSqlError error = this->Internals->QtQuery.lastError();
  if (error.isValid())
  {
    QString errorString;
    errorString.sprintf(
      "Query execute error: %s (type:%d)\n", error.text().toLatin1().data(), error.type());
    svtkErrorMacro(<< errorString.toLatin1().data());
    return false;
  }

  // cache the column names
  this->Internals->FieldNames.clear();
  for (int i = 0; i < this->Internals->QtQuery.record().count(); i++)
  {
    this->Internals->FieldNames.push_back(
      this->Internals->QtQuery.record().fieldName(i).toLatin1().data());
  }
  return true;
}

int svtkQtSQLQuery::GetNumberOfFields()
{
  return this->Internals->QtQuery.record().count();
}

const char* svtkQtSQLQuery::GetFieldName(int col)
{
  return this->Internals->FieldNames[col].c_str();
}

int QVariantTypeToSVTKType(QVariant::Type t)
{
  int type = -1;
  switch (t)
  {
    case QVariant::Bool:
      type = SVTK_INT;
      break;
    case QVariant::Char:
      type = SVTK_CHAR;
      break;
    case QVariant::DateTime:
    case QVariant::Date:
    case QVariant::Time:
      type = SVTK_TYPE_UINT64;
      break;
    case QVariant::Double:
      type = SVTK_DOUBLE;
      break;
    case QVariant::Int:
      type = SVTK_INT;
      break;
    case QVariant::UInt:
      type = SVTK_UNSIGNED_INT;
      break;
    case QVariant::LongLong:
      type = SVTK_TYPE_INT64;
      break;
    case QVariant::ULongLong:
      type = SVTK_TYPE_UINT64;
      break;
    case QVariant::String:
      type = SVTK_STRING;
      break;
    case QVariant::ByteArray:
      type = SVTK_STRING;
      break;
    case QVariant::Invalid:
    default:
      cerr << "Found unknown variant type: " << t << endl;
      type = -1;
  }
  return type;
}

int svtkQtSQLQuery::GetFieldType(int col)
{
  return QVariantTypeToSVTKType(this->Internals->QtQuery.record().field(col).type());
}

bool svtkQtSQLQuery::NextRow()
{
  return this->Internals->QtQuery.next();
}

svtkVariant svtkQtSQLQuery::DataValue(svtkIdType c)
{
  QVariant v = this->Internals->QtQuery.value(c);
  switch (v.type())
  {
    case QVariant::Bool:
      return svtkVariant(v.toInt());
    case QVariant::Char:
      return svtkVariant(v.toChar().toLatin1());
    case QVariant::DateTime:
    {
      QDateTime dt = v.toDateTime();
      svtkTypeUInt64 timePoint = svtkQtTimePointUtility::QDateTimeToTimePoint(dt);
      return svtkVariant(timePoint);
    }
    case QVariant::Date:
    {
      QDate date = v.toDate();
      svtkTypeUInt64 timePoint = svtkQtTimePointUtility::QDateToTimePoint(date);
      return svtkVariant(timePoint);
    }
    case QVariant::Time:
    {
      QTime time = v.toTime();
      svtkTypeUInt64 timePoint = svtkQtTimePointUtility::QTimeToTimePoint(time);
      return svtkVariant(timePoint);
    }
    case QVariant::Double:
      return svtkVariant(v.toDouble());
    case QVariant::Int:
      return svtkVariant(v.toInt());
    case QVariant::LongLong:
      return svtkVariant(v.toLongLong());
    case QVariant::String:
      return svtkVariant(v.toString().toLatin1().data());
    case QVariant::UInt:
      return svtkVariant(v.toUInt());
    case QVariant::ULongLong:
      return svtkVariant(v.toULongLong());
    case QVariant::ByteArray:
    {
      // Carefully storing BLOBs as svtkStrings. This
      // avoids the normal termination problems with
      // zero's in the BLOBs...
      return svtkVariant(svtkStdString(v.toByteArray().data(), v.toByteArray().length()));
    }
    case QVariant::Invalid:
      return svtkVariant();
    default:
      svtkErrorMacro(<< "Unhandled Qt variant type " << v.type()
                    << " found; returning string variant.");
      return svtkVariant(v.toString().toLatin1().data());
  }
}

#endif // (QT_EDITION & QT_MODULE_SQL)
