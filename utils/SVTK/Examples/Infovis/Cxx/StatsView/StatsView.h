/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StatsView.h
  Language:  C++

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef StatsView_H
#define StatsView_H

#include "svtkSmartPointer.h" // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_StatsView;

// Forward SVTK class declarations
class svtkRowQueryToTable;
class svtkQtTableView;

class StatsView : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  StatsView();
  ~StatsView() override;

public slots:

  virtual void slotOpenSQLiteDB();

protected:
protected slots:

private:
  // Methods
  void SetupSelectionLink();

  // Members
  svtkSmartPointer<svtkRowQueryToTable> RowQueryToTable;
  svtkSmartPointer<svtkQtTableView> TableView1;
  svtkSmartPointer<svtkQtTableView> TableView2;
  svtkSmartPointer<svtkQtTableView> TableView3;
  svtkSmartPointer<svtkQtTableView> TableView4;

  // Designer form
  Ui_StatsView* ui;
};

#endif // StatsView_H
