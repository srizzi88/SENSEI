/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "SimpleView.h"
#include "ui_SimpleView.h"

#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkSmartPointer.h"
#include <svtkDataObjectToTable.h>
#include <svtkElevationFilter.h>
#include <svtkNew.h>
#include <svtkPolyDataMapper.h>
#include <svtkQtTableView.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>
#include <svtkVectorText.h>

// Constructor
SimpleView::SimpleView()
{
  this->ui = new Ui_SimpleView;
  this->ui->setupUi(this);

  // Qt Table View
  this->TableView = svtkSmartPointer<svtkQtTableView>::New();

  // Place the table view in the designer form
  this->ui->tableFrame->layout()->addWidget(this->TableView->GetWidget());

  // Geometry
  svtkNew<svtkVectorText> text;
  text->SetText("SVTK and Qt!");
  svtkNew<svtkElevationFilter> elevation;
  elevation->SetInputConnection(text->GetOutputPort());
  elevation->SetLowPoint(0, 0, 0);
  elevation->SetHighPoint(10, 0, 0);

  // Mapper
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elevation->GetOutputPort());

  // Actor in scene
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // SVTK Renderer
  svtkNew<svtkRenderer> ren;

  // Add Actor to renderer
  ren->AddActor(actor);

  // SVTK/Qt wedded
  svtkNew<svtkGenericOpenGLRenderWindow> renderWindow;
  this->ui->qsvtkWidget->setRenderWindow(renderWindow);
  this->ui->qsvtkWidget->renderWindow()->AddRenderer(ren);

  // Just a bit of Qt interest: Culling off the
  // point data and handing it to a svtkQtTableView
  svtkNew<svtkDataObjectToTable> toTable;
  toTable->SetInputConnection(elevation->GetOutputPort());
  toTable->SetFieldType(svtkDataObjectToTable::POINT_DATA);

  // Here we take the end of the SVTK pipeline and give it to a Qt View
  this->TableView->SetRepresentationFromInputConnection(toTable->GetOutputPort());

  // Set up action signals and slots
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(slotOpenFile()));
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
};

SimpleView::~SimpleView()
{
  // The smart pointers should clean up for up
}

// Action to be taken upon file open
void SimpleView::slotOpenFile() {}

void SimpleView::slotExit()
{
  qApp->exit();
}
