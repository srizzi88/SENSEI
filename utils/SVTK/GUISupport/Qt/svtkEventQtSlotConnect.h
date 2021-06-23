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

/**
 * @class   svtkEventQtSlotConnect
 * @brief   Manage connections between SVTK events and Qt slots.
 *
 * svtkEventQtSlotConnect provides a way to manage connections between SVTK events
 * and Qt slots.
 * Qt slots to connect with must have one of the following signatures:
 * - MySlot()
 * - MySlot(svtkObject* caller)
 * - MySlot(svtkObject* caller, unsigned long svtk_event)
 * - MySlot(svtkObject* caller, unsigned long svtk_event, void* client_data)
 * - MySlot(svtkObject* caller, unsigned long svtk_event, void* client_data, void* call_data)
 * - MySlot(svtkObject* caller, unsigned long svtk_event, void* client_data, void* call_data,
 * svtkCommand*)
 */

#ifndef svtkEventQtSlotConnect_h
#define svtkEventQtSlotConnect_h

#include "QSVTKWin32Header.h"       // for export define
#include "svtkCommand.h"            // for event defines
#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkObject.h"
#include <QtCore/QObject> // for version info

class QObject;
class svtkQtConnections;
class svtkQtConnection;

// manage connections between SVTK object events and Qt slots
class SVTKGUISUPPORTQT_EXPORT svtkEventQtSlotConnect : public svtkObject
{
public:
  static svtkEventQtSlotConnect* New();
  svtkTypeMacro(svtkEventQtSlotConnect, svtkObject);

  /**
   * Print the current connections between SVTK and Qt
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Connect a svtk object's event with a Qt object's slot.  Multiple
   * connections which are identical are treated as separate connections.
   */
  virtual void Connect(svtkObject* svtk_obj, unsigned long event, const QObject* qt_obj,
    const char* slot, void* client_data = nullptr, float priority = 0.0,
    Qt::ConnectionType type = Qt::AutoConnection);

  /**
   * Disconnect a svtk object from a qt object.
   * Passing no arguments will disconnect all slots maintained by this object.
   * Passing in only a svtk object will disconnect all slots from it.
   * Passing only a svtk object and event, will disconnect all slots matching
   * the svtk object and event.
   * Passing all information in will match all information.
   */
  virtual void Disconnect(svtkObject* svtk_obj = nullptr, unsigned long event = svtkCommand::NoEvent,
    const QObject* qt_obj = nullptr, const char* slot = nullptr, void* client_data = nullptr);

  /**
   * Allow to query svtkEventQtSlotConnect to know if some Connect() have been
   * setup and how many.
   */
  virtual int GetNumberOfConnections() const;

protected:
  svtkQtConnections* Connections;
  friend class svtkQtConnection;
  void RemoveConnection(svtkQtConnection*);

  svtkEventQtSlotConnect();
  ~svtkEventQtSlotConnect() override;

private:
  svtkEventQtSlotConnect(const svtkEventQtSlotConnect&) = delete;
  void operator=(const svtkEventQtSlotConnect&) = delete;
};

#endif
