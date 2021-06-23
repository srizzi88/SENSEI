/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CustomLinkView.h
  Language:  C++

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

// .NAME CustomLinkView - Shows custom way of linking multiple views.
//
// .SECTION Description
// CustomLinkView shows an alternate way to link various views using
// svtkEventQtSlotConnect where selection in a particular view sets
// the same selection in all other views associated.

// Other way to get the same functionality is by using svtkAnnotationLink
// shared between multiple views.

// .SECTION See Also
// EasyView

#ifndef CustomLinkView_H
#define CustomLinkView_H

#include "svtkSmartPointer.h" // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_CustomLinkView;

// Forward SVTK class declarations
class svtkCommand;
class svtkEventQtSlotConnect;
class svtkGraphLayoutView;
class svtkObject;
class svtkQtTableView;
class svtkQtTreeView;
class svtkXMLTreeReader;

class CustomLinkView : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  CustomLinkView();
  ~CustomLinkView() override;

public slots:

  virtual void slotOpenXMLFile();
  virtual void slotExit();

protected:
protected slots:

public slots:
  // Qt signal (produced by svtkEventQtSlotConnect) will be connected to
  // this slot.
  // Full signature of the slot could be:
  // MySlot(svtkObject* caller, unsigned long svtk_event,
  //         void* clientData, void* callData, svtkCommand*)
  void selectionChanged(svtkObject*, unsigned long, void*, void* callData);

private:
  // Methods
  void SetupCustomLink();

  // Members
  svtkSmartPointer<svtkXMLTreeReader> XMLReader;
  svtkSmartPointer<svtkGraphLayoutView> GraphView;
  svtkSmartPointer<svtkQtTreeView> TreeView;
  svtkSmartPointer<svtkQtTableView> TableView;
  svtkSmartPointer<svtkQtTreeView> ColumnView;

  // This class converts a svtkEvent to QT signal.
  svtkSmartPointer<svtkEventQtSlotConnect> Connections;

  // Designer form
  Ui_CustomLinkView* ui;
};

#endif // CustomLinkView_H
