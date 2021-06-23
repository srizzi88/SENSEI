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

// .SECTION Description
// svtkQtConnection is an internal class.

#ifndef svtkQtConnection_h
#define svtkQtConnection_h

#include "svtkCommand.h" // for event defines
#include <QObject>

class svtkObject;
class svtkCallbackCommand;
class svtkEventQtSlotConnect;

// class for managing a single SVTK/Qt connection
// not to be included in other projects
// only here for moc to process for svtkEventQtSlotConnect
class svtkQtConnection : public QObject
{
  Q_OBJECT

public:
  // constructor
  svtkQtConnection(svtkEventQtSlotConnect* owner);

  // destructor, disconnect if necessary
  ~svtkQtConnection() override;

  // print function
  void PrintSelf(ostream& os, svtkIndent indent);

  // callback from SVTK to emit signal
  void Execute(svtkObject* caller, unsigned long event, void* client_data);

  // set the connection
  void SetConnection(svtkObject* svtk_obj, unsigned long event, const QObject* qt_obj,
    const char* slot, void* client_data, float priority = 0.0,
    Qt::ConnectionType type = Qt::AutoConnection);

  // check if a connection matches input parameters
  bool IsConnection(svtkObject* svtk_obj, unsigned long event, const QObject* qt_obj,
    const char* slot, void* client_data);

  static void DoCallback(
    svtkObject* svtk_obj, unsigned long event, void* client_data, void* call_data);

signals:
  // the qt signal for moc to take care of
  void EmitExecute(svtkObject*, unsigned long, void* client_data, void* call_data, svtkCommand*);

protected slots:
  void deleteConnection();

protected:
  // the connection information
  svtkObject* SVTKObject;
  svtkCallbackCommand* Callback;
  const QObject* QtObject;
  void* ClientData;
  unsigned long SVTKEvent;
  QString QtSlot;
  svtkEventQtSlotConnect* Owner;

private:
  svtkQtConnection(const svtkQtConnection&);
  void operator=(const svtkQtConnection&);
};

#endif
// SVTK-HeaderTest-Exclude: svtkQtConnection.h
