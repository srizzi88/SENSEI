/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtSQLDatabase.cxx

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

#include "svtkQtSQLDatabase.h"

#include "svtkObjectFactory.h"
#include "svtkQtSQLQuery.h"

#include "svtkStringArray.h"
#include "svtkVariant.h"

#include <QtSql/QSqlError>
#include <QtSql/QtSql>

#include <sstream>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkQtSQLDatabase);

int svtkQtSQLDatabase::id = 0;

svtkSQLDatabase* svtkQtSQLDatabaseCreateFromURLCallback(const char* URL)
{
  return svtkQtSQLDatabase::CreateFromURL(URL);
}

class svtkQtSQLDatabaseInitializer
{
public:
  inline void Use() {}

  svtkQtSQLDatabaseInitializer()
  {
    svtkSQLDatabase::RegisterCreateFromURLCallback(svtkQtSQLDatabaseCreateFromURLCallback);
  }
};

static svtkQtSQLDatabaseInitializer svtkQtSQLDatabaseInitializerGlobal;

svtkQtSQLDatabase::svtkQtSQLDatabase()
{
  svtkQtSQLDatabaseInitializerGlobal.Use();
  this->DatabaseType = nullptr;
  this->HostName = nullptr;
  this->UserName = nullptr;
  this->DatabaseName = nullptr;
  this->Port = -1;
  this->ConnectOptions = nullptr;
  this->myTables = svtkStringArray::New();
  this->currentRecord = svtkStringArray::New();
}

svtkQtSQLDatabase::~svtkQtSQLDatabase()
{
  this->SetDatabaseType(nullptr);
  this->SetHostName(nullptr);
  this->SetUserName(nullptr);
  this->SetDatabaseName(nullptr);
  this->SetConnectOptions(nullptr);
  this->myTables->Delete();
  this->currentRecord->Delete();
}

bool svtkQtSQLDatabase::Open(const char* password)
{
  if (!QCoreApplication::instance())
  {
    svtkErrorMacro("Qt isn't initialized, you must create an instance of QCoreApplication before "
                  "using this class.");
    return false;
  }

  if (this->DatabaseType == nullptr)
  {
    svtkErrorMacro("Qt database type must be non-null.");
    return false;
  }

  // We have to assign a unique ID to each database connection, so
  // Qt doesn't blow-away existing connections
  const QString connection_name = QString::number(this->id++);
  this->QtDatabase = QSqlDatabase::addDatabase(this->DatabaseType, connection_name);

  if (this->HostName != nullptr)
  {
    this->QtDatabase.setHostName(this->HostName);
  }
  if (this->DatabaseName != nullptr)
  {
    this->QtDatabase.setDatabaseName(this->DatabaseName);
  }
  if (this->ConnectOptions != nullptr)
  {
    this->QtDatabase.setConnectOptions(this->ConnectOptions);
  }
  if (this->Port >= 0)
  {
    this->QtDatabase.setPort(this->Port);
  }
  if (this->QtDatabase.open(this->UserName, password))
  {
    return true;
  }

  return false;
}

void svtkQtSQLDatabase::Close()
{
  this->QtDatabase.close();
}

bool svtkQtSQLDatabase::IsOpen()
{
  return this->QtDatabase.isOpen();
}

svtkSQLQuery* svtkQtSQLDatabase::GetQueryInstance()
{
  svtkQtSQLQuery* query = svtkQtSQLQuery::New();
  query->SetDatabase(this);
  return query;
}

bool svtkQtSQLDatabase::HasError()
{
  return (this->QtDatabase.lastError().number() != QSqlError::NoError);
}

const char* svtkQtSQLDatabase::GetLastErrorText()
{
  return this->QtDatabase.lastError().text().toLatin1();
}

svtkStringArray* svtkQtSQLDatabase::GetTables()
{
  // Clear out any exiting stuff
  this->myTables->Initialize();

  // Yea... do different things depending on database type
  // Get tables on oracle is different
  if (this->QtDatabase.driverName() == "QOCI")
  {
    svtkSQLQuery* query = this->GetQueryInstance();
    query->SetQuery("select table_name from user_tables");
    query->Execute();
    while (query->NextRow())
      this->myTables->InsertNextValue(query->DataValue(0).ToString());

    // Okay done with query so delete
    query->Delete();
  }
  else
  {
    // Copy the table list from Qt database
    QStringList tables = this->QtDatabase.tables(QSql::Tables);
    for (int i = 0; i < tables.size(); ++i)
    {
      this->myTables->InsertNextValue(tables.at(i).toLatin1());
    }
  }

  return this->myTables;
}

svtkStringArray* svtkQtSQLDatabase::GetRecord(const char* table)
{
  // Clear any existing records
  currentRecord->Resize(0);

  QSqlRecord columns = this->QtDatabase.record(table);
  for (int i = 0; i < columns.count(); i++)
  {
    this->currentRecord->InsertNextValue(columns.fieldName(i).toLatin1());
  }

  return currentRecord;
}

svtkStringArray* svtkQtSQLDatabase::GetColumns()
{
  return this->currentRecord;
}

void svtkQtSQLDatabase::SetColumnsTable(const char* table)
{
  this->GetRecord(table);
}

bool svtkQtSQLDatabase::IsSupported(int feature)
{
  switch (feature)
  {
    case SVTK_SQL_FEATURE_TRANSACTIONS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::Transactions);

    case SVTK_SQL_FEATURE_QUERY_SIZE:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::QuerySize);

    case SVTK_SQL_FEATURE_BLOB:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::BLOB);

    case SVTK_SQL_FEATURE_UNICODE:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::Unicode);

    case SVTK_SQL_FEATURE_PREPARED_QUERIES:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::PreparedQueries);

    case SVTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::NamedPlaceholders);

    case SVTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::PositionalPlaceholders);

    case SVTK_SQL_FEATURE_LAST_INSERT_ID:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::LastInsertId);

    case SVTK_SQL_FEATURE_BATCH_OPERATIONS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::BatchOperations);

    default:
    {
      svtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                    << "svtkSQLDatabase.h for a list of possible features.");
      return false;
    }
  }
}

void svtkQtSQLDatabase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "nullptr") << endl;
  os << indent << "HostName: " << (this->HostName ? this->HostName : "nullptr") << endl;
  os << indent << "UserName: " << (this->UserName ? this->UserName : "nullptr") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "nullptr") << endl;
  os << indent << "Port: " << this->Port << endl;
  os << indent << "ConnectOptions: " << (this->ConnectOptions ? this->ConnectOptions : "nullptr")
     << endl;
}

// ----------------------------------------------------------------------
bool svtkQtSQLDatabase::ParseURL(const char* URL)
{
  std::string protocol;
  std::string username;
  std::string unused;
  std::string hostname;
  std::string dataport;
  std::string database;
  std::string dataglom;

  // SQLite is a bit special so lets get that out of the way :)
  if (!svtksys::SystemTools::ParseURLProtocol(URL, protocol, dataglom))
  {
    svtkGenericWarningMacro("Invalid URL: " << URL);
    return false;
  }

  if (protocol == "sqlite")
  {
    this->SetDatabaseType("QSQLITE");
    this->SetDatabaseName(dataglom.c_str());
    return true;
  }

  // Okay now for all the other database types get more detailed info
  if (!svtksys::SystemTools::ParseURL(URL, protocol, username, unused, hostname, dataport, database))
  {
    svtkGenericWarningMacro("Invalid URL: " << URL);
    return false;
  }

  // Create Qt 'version' of database prototcol type
  QString qtType;
  qtType = protocol.c_str();
  qtType = "Q" + qtType.toUpper();

  this->SetDatabaseType(qtType.toLatin1());
  this->SetUserName(username.c_str());
  this->SetHostName(hostname.c_str());
  this->SetPort(atoi(dataport.c_str()));
  this->SetDatabaseName(database.c_str());
  return true;
}

// ----------------------------------------------------------------------
svtkSQLDatabase* svtkQtSQLDatabase::CreateFromURL(const char* URL)
{
  svtkQtSQLDatabase* qt_db = svtkQtSQLDatabase::New();
  if (qt_db->ParseURL(URL))
  {
    return qt_db;
  }
  qt_db->Delete();
  return nullptr;
}

// ----------------------------------------------------------------------
svtkStdString svtkQtSQLDatabase::GetURL()
{
  svtkStdString url;
  url = this->GetDatabaseType();
  url += "://";
  url += this->GetUserName();
  url += "@";
  url += this->GetHostName();
  url += ":";
  url += this->GetPort();
  url += "/";
  url += this->GetDatabaseName();
  return url;
}

#endif // (QT_EDITION & QT_MODULE_SQL)
