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

#include "svtkQtConnection.h"
#include "svtkCallbackCommand.h"
#include "svtkEventQtSlotConnect.h"

#include <qmetaobject.h>
#include <qobject.h>

// constructor
svtkQtConnection::svtkQtConnection(svtkEventQtSlotConnect* owner)
  : Owner(owner)
{
  this->Callback = svtkCallbackCommand::New();
  this->Callback->SetCallback(svtkQtConnection::DoCallback);
  this->Callback->SetClientData(this);
  this->SVTKObject = nullptr;
  this->QtObject = nullptr;
  this->ClientData = nullptr;
  this->SVTKEvent = svtkCommand::NoEvent;
}

// destructor, disconnect if necessary
svtkQtConnection::~svtkQtConnection()
{
  if (this->SVTKObject)
  {
    this->SVTKObject->RemoveObserver(this->Callback);
    // Qt takes care of disconnecting slots
  }
  this->Callback->Delete();
}

void svtkQtConnection::DoCallback(
  svtkObject* svtk_obj, unsigned long event, void* client_data, void* call_data)
{
  svtkQtConnection* conn = static_cast<svtkQtConnection*>(client_data);
  conn->Execute(svtk_obj, event, call_data);
}

// callback from SVTK to emit signal
void svtkQtConnection::Execute(svtkObject* caller, unsigned long e, void* call_data)
{
  if (e != svtkCommand::DeleteEvent || (this->SVTKEvent == svtkCommand::DeleteEvent))
  {
    emit EmitExecute(caller, e, ClientData, call_data, this->Callback);
  }

  if (e == svtkCommand::DeleteEvent)
  {
    this->Owner->Disconnect(this->SVTKObject, this->SVTKEvent, this->QtObject,
      this->QtSlot.toLatin1().data(), this->ClientData);
  }
}

bool svtkQtConnection::IsConnection(
  svtkObject* svtk_obj, unsigned long e, const QObject* qt_obj, const char* slot, void* client_data)
{
  if (this->SVTKObject != svtk_obj)
    return false;

  if (e != svtkCommand::NoEvent && e != this->SVTKEvent)
    return false;

  if (qt_obj && qt_obj != this->QtObject)
    return false;

  if (slot && this->QtSlot != slot)
    return false;

  if (client_data && this->ClientData != client_data)
    return false;

  return true;
}

// set the connection
void svtkQtConnection::SetConnection(svtkObject* svtk_obj, unsigned long e, const QObject* qt_obj,
  const char* slot, void* client_data, float priority, Qt::ConnectionType type)
{
  // keep track of what we connected
  this->SVTKObject = svtk_obj;
  this->QtObject = qt_obj;
  this->SVTKEvent = e;
  this->ClientData = client_data;
  this->QtSlot = slot;

  // make a connection between this and the svtk object
  svtk_obj->AddObserver(e, this->Callback, priority);

  if (e != svtkCommand::DeleteEvent)
  {
    svtk_obj->AddObserver(svtkCommand::DeleteEvent, this->Callback);
  }

  // make a connection between this and the Qt object
  qt_obj->connect(
    this, SIGNAL(EmitExecute(svtkObject*, unsigned long, void*, void*, svtkCommand*)), slot, type);
  QObject::connect(qt_obj, SIGNAL(destroyed(QObject*)), this, SLOT(deleteConnection()));
}

void svtkQtConnection::deleteConnection()
{
  this->Owner->RemoveConnection(this);
}

void svtkQtConnection::PrintSelf(ostream& os, svtkIndent indent)
{
  if (this->SVTKObject && this->QtObject)
  {
    os << indent << this->SVTKObject->GetClassName() << ":"
       << svtkCommand::GetStringFromEventId(this->SVTKEvent) << "  <---->  "
       << this->QtObject->metaObject()->className() << "::" << this->QtSlot.toLatin1().data()
       << "\n";
  }
}
