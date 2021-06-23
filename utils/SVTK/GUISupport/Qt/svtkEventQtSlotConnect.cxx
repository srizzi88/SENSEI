/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using SVTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/svtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like SVTK
 !!! license.
=========================================================================*/

#include "svtkEventQtSlotConnect.h"
#include "svtkCallbackCommand.h"
#include "svtkObjectFactory.h"
#include "svtkQtConnection.h"

#include <vector>

#include <qobject.h>

// hold all the connections
class svtkQtConnections : public std::vector<svtkQtConnection*>
{
};

svtkStandardNewMacro(svtkEventQtSlotConnect);

// constructor
svtkEventQtSlotConnect::svtkEventQtSlotConnect()
{
  Connections = new svtkQtConnections;
}

svtkEventQtSlotConnect::~svtkEventQtSlotConnect()
{
  // clean out connections
  svtkQtConnections::iterator iter;
  for (iter = Connections->begin(); iter != Connections->end(); ++iter)
  {
    delete (*iter);
  }

  delete Connections;
}

void svtkEventQtSlotConnect::Connect(svtkObject* svtk_obj, unsigned long event, const QObject* qt_obj,
  const char* slot, void* client_data, float priority, Qt::ConnectionType type)
{
  if (!svtk_obj || !qt_obj)
  {
    svtkErrorMacro("Cannot connect null objects.");
    return;
  }
  svtkQtConnection* connection = new svtkQtConnection(this);
  connection->SetConnection(svtk_obj, event, qt_obj, slot, client_data, priority, type);
  Connections->push_back(connection);
}

void svtkEventQtSlotConnect::Disconnect(svtkObject* svtk_obj, unsigned long event,
  const QObject* qt_obj, const char* slot, void* client_data)
{
  if (!svtk_obj)
  {
    svtkQtConnections::iterator iter;
    for (iter = this->Connections->begin(); iter != this->Connections->end(); ++iter)
    {
      delete (*iter);
    }
    this->Connections->clear();
    return;
  }
  bool all_info = true;
  if (slot == nullptr || qt_obj == nullptr || event == svtkCommand::NoEvent)
  {
    all_info = false;
  }

  svtkQtConnections::iterator iter;
  for (iter = Connections->begin(); iter != Connections->end();)
  {
    // if information matches, remove the connection
    if ((*iter)->IsConnection(svtk_obj, event, qt_obj, slot, client_data))
    {
      delete (*iter);
      iter = Connections->erase(iter);
      // if user passed in all information, only remove one connection and quit
      if (all_info)
      {
        iter = Connections->end();
      }
    }
    else
    {
      ++iter;
    }
  }
}

void svtkEventQtSlotConnect::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (Connections->empty())
  {
    os << indent << "No Connections\n";
  }
  else
  {
    os << indent << "Connections:\n";
    svtkQtConnections::iterator iter;
    for (iter = Connections->begin(); iter != Connections->end(); ++iter)
    {
      (*iter)->PrintSelf(os, indent.GetNextIndent());
    }
  }
}

void svtkEventQtSlotConnect::RemoveConnection(svtkQtConnection* conn)
{
  svtkQtConnections::iterator iter;
  for (iter = this->Connections->begin(); iter != this->Connections->end(); ++iter)
  {
    if (conn == *iter)
    {
      delete (*iter);
      Connections->erase(iter);
      return;
    }
  }
}

int svtkEventQtSlotConnect::GetNumberOfConnections() const
{
  return static_cast<int>(this->Connections->size());
}
