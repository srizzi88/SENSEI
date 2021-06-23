/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtAnnotationView.cxx

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

#include "svtkQtAnnotationView.h"

#include <QHeaderView>
#include <QItemSelection>
#include <QTableView>

#include "svtkAbstractArray.h"
#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkAnnotationLink.h"
#include "svtkCommand.h"
#include "svtkConvertSelection.h"
#include "svtkDataRepresentation.h"
#include "svtkDataSetAttributes.h"
#include "svtkEventQtSlotConnect.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkQtAnnotationLayersModelAdapter.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

svtkStandardNewMacro(svtkQtAnnotationView);

//----------------------------------------------------------------------------
svtkQtAnnotationView::svtkQtAnnotationView()
{
  this->View = new QTableView();
  this->Adapter = new svtkQtAnnotationLayersModelAdapter();
  this->View->setModel(this->Adapter);

  // Set up some default properties
  this->View->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->View->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->View->setAlternatingRowColors(true);
  this->View->setSortingEnabled(true);
  this->View->setDragEnabled(true);
  this->View->setDragDropMode(QAbstractItemView::InternalMove);
  this->View->setDragDropOverwriteMode(false);
  this->View->setAcceptDrops(true);
  this->View->setDropIndicatorShown(true);
  this->View->horizontalHeader()->show();

  this->LastInputMTime = 0;

  QObject::connect(this->View->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));
}

//----------------------------------------------------------------------------
svtkQtAnnotationView::~svtkQtAnnotationView()
{
  delete this->View;
  delete this->Adapter;
}

//----------------------------------------------------------------------------
QWidget* svtkQtAnnotationView::GetWidget()
{
  return this->View;
}

//----------------------------------------------------------------------------
void svtkQtAnnotationView::slotQtSelectionChanged(
  const QItemSelection& svtkNotUsed(s1), const QItemSelection& svtkNotUsed(s2))
{
  svtkDataObject* data = this->Adapter->GetSVTKDataObject();
  if (!data)
    return;

  QModelIndexList qmi = this->View->selectionModel()->selectedRows();
  svtkAnnotationLayers* curLayers =
    this->GetRepresentation()->GetAnnotationLink()->GetAnnotationLayers();
  for (unsigned int i = 0; i < curLayers->GetNumberOfAnnotations(); ++i)
  {
    svtkAnnotation* a = curLayers->GetAnnotation(i);
    svtkAnnotation::ENABLE()->Set(a->GetInformation(), 0);
  }

  for (int j = 0; j < qmi.count(); ++j)
  {
    svtkAnnotation* a = curLayers->GetAnnotation(qmi[j].row());
    svtkAnnotation::ENABLE()->Set(a->GetInformation(), 1);
  }

  // svtkSmartPointer<svtkAnnotationLayers> annotations;
  // annotations.TakeReference(this->Adapter->QModelIndexListToSVTKAnnotationLayers(qmi));
  // for(int i=0; i<annotations->GetNumberOfAnnotations(); ++i)
  //  {
  //  svtkAnnotation* a = annotations->GetAnnotation(i);
  //  a->ENABLED().Set(1);
  //  }
  // this->GetRepresentation()->GetAnnotationLink()->SetAnnotationLayers(annotations);
  this->InvokeEvent(svtkCommand::AnnotationChangedEvent, reinterpret_cast<void*>(curLayers));

  this->LastInputMTime =
    this->GetRepresentation()->GetAnnotationLink()->GetAnnotationLayers()->GetMTime();
}

//----------------------------------------------------------------------------
void svtkQtAnnotationView::Update()
{
  svtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
  {
    this->Adapter->reset();
    this->View->update();
    return;
  }

  // Make sure the input connection is up to date.
  svtkDataObject* a = rep->GetAnnotationLink()->GetAnnotationLayers();
  if (a->GetMTime() != this->LastInputMTime)
  {
    this->LastInputMTime = a->GetMTime();

    this->Adapter->SetSVTKDataObject(nullptr);
    this->Adapter->SetSVTKDataObject(a);
  }

  this->View->update();

  this->View->resizeColumnToContents(0);
  this->View->resizeColumnToContents(1);
}

//----------------------------------------------------------------------------
void svtkQtAnnotationView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
