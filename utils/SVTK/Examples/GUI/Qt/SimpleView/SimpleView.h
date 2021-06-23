/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SimpleView.h
  Language:  C++

  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef SimpleView_H
#define SimpleView_H

#include "svtkSmartPointer.h" // Required for smart pointer internal ivars.
#include <QMainWindow>

// Forward Qt class declarations
class Ui_SimpleView;

// Forward SVTK class declarations
class svtkQtTableView;

class SimpleView : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  SimpleView();
  ~SimpleView() override;

public slots:

  virtual void slotOpenFile();
  virtual void slotExit();

protected:
protected slots:

private:
  svtkSmartPointer<svtkQtTableView> TableView;

  // Designer form
  Ui_SimpleView* ui;
};

#endif // SimpleView_H
