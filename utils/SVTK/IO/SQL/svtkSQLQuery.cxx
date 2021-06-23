/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLQuery.cxx

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
#include "svtkSQLQuery.h"

#include "svtkObjectFactory.h"
#include "svtkSQLDatabase.h"
#include "svtkVariantArray.h"

#include "svtksys/SystemTools.hxx"

svtkSQLQuery::svtkSQLQuery()
{
  this->Query = nullptr;
  this->Database = nullptr;
  this->Active = false;
}

svtkSQLQuery::~svtkSQLQuery()
{
  this->SetQuery(nullptr);
  if (this->Database)
  {
    this->Database->Delete();
    this->Database = nullptr;
  }
}

svtkCxxSetObjectMacro(svtkSQLQuery, Database, svtkSQLDatabase);

void svtkSQLQuery::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query: " << (this->Query ? this->Query : "nullptr") << endl;
  os << indent << "Database: " << (this->Database ? "" : "nullptr") << endl;
  if (this->Database)
  {
    this->Database->PrintSelf(os, indent.GetNextIndent());
  }
}

svtkStdString svtkSQLQuery::EscapeString(svtkStdString s, bool addSurroundingQuotes)
{
  svtkStdString d;
  if (addSurroundingQuotes)
  {
    d += '\'';
  }

  for (svtkStdString::iterator it = s.begin(); it != s.end(); ++it)
  {
    if (*it == '\'')
      d += '\''; // Single quotes are escaped by repeating them
    d += *it;
  }

  if (addSurroundingQuotes)
  {
    d += '\'';
  }
  return d;
}

char* svtkSQLQuery::EscapeString(const char* src, bool addSurroundingQuotes)
{
  svtkStdString sstr(src);
  svtkStdString dstr = this->EscapeString(sstr, addSurroundingQuotes);
  return svtksys::SystemTools::DuplicateString(dstr.c_str());
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), unsigned char svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), signed char svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), unsigned short svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), short svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), unsigned int svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), int svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), unsigned long svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), long svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), unsigned long long svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), long long svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), float svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), double svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), const char* svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(
  int svtkNotUsed(index), const char* svtkNotUsed(value), size_t svtkNotUsed(length))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int svtkNotUsed(index), const svtkStdString& svtkNotUsed(value))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(
  int svtkNotUsed(index), const void* svtkNotUsed(value), size_t svtkNotUsed(length))
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::ClearParameterBindings()
{
  svtkErrorMacro(<< "This database driver does not support bound parameters.");
  return false;
}

bool svtkSQLQuery::BindParameter(int index, svtkVariant data)
{
  if (!data.IsValid())
  {
    return true; // binding nulls is a no-op
  }

#define SVTK_VARIANT_BIND_PARAMETER(Type, Function)                                                 \
  case Type:                                                                                       \
    return this->BindParameter(index, data.Function())

  switch (data.GetType())
  {
    SVTK_VARIANT_BIND_PARAMETER(SVTK_STRING, ToString);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_FLOAT, ToFloat);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_DOUBLE, ToDouble);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_CHAR, ToChar);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_UNSIGNED_CHAR, ToUnsignedChar);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_SIGNED_CHAR, ToSignedChar);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_SHORT, ToShort);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_UNSIGNED_SHORT, ToUnsignedShort);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_INT, ToInt);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_UNSIGNED_INT, ToUnsignedInt);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_LONG, ToLong);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_UNSIGNED_LONG, ToUnsignedLong);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_LONG_LONG, ToLongLong);
    SVTK_VARIANT_BIND_PARAMETER(SVTK_UNSIGNED_LONG_LONG, ToUnsignedLongLong);
    case SVTK_OBJECT:
      svtkErrorMacro(<< "Variants of type SVTK_OBJECT cannot be inserted into a database.");
      return false;
    default:
      svtkErrorMacro(<< "Variants of type " << data.GetType()
                    << " are not currently supported by BindParameter.");
      return false;
  }
}

// ----------------------------------------------------------------------

bool svtkSQLQuery::SetQuery(const char* queryString)
{
  // This is just svtkSetStringMacro from svtkSetGet.h

  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Query to "
                << (queryString ? queryString : "(null)"));

  if (this->Query == nullptr && queryString == nullptr)
  {
    return true;
  }
  if (this->Query && queryString && (!strcmp(this->Query, queryString)))
  {
    return true; // query string isn't changing
  }
  delete[] this->Query;
  if (queryString)
  {
    size_t n = strlen(queryString) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (queryString);
    this->Query = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->Query = nullptr;
  }
  this->Modified();
  return true;
}

// As above, this is a copy of svtkGetStringMacro from svtkGetSet.h.

const char* svtkSQLQuery::GetQuery()
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Query of "
                << (this->Query ? this->Query : "(null)"));
  return this->Query;
}
