/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtRecordView.cxx

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

#include "svtkQtRecordView.h"
#include <QObject>
#include <QTextEdit>

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAnnotationLink.h"
#include "svtkCommand.h"
#include "svtkConvertSelection.h"
#include "svtkDataObjectToTable.h"
#include "svtkDataRepresentation.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkQtRecordView);

//----------------------------------------------------------------------------
svtkQtRecordView::svtkQtRecordView()
{
  this->TextWidget = new QTextEdit();
  this->DataObjectToTable = svtkSmartPointer<svtkDataObjectToTable>::New();
  this->DataObjectToTable->SetFieldType(svtkDataObjectToTable::VERTEX_DATA);
  this->FieldType = svtkQtRecordView::VERTEX_DATA;
  this->Text = nullptr;
  this->CurrentSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
}

//----------------------------------------------------------------------------
svtkQtRecordView::~svtkQtRecordView()
{
  delete this->TextWidget;
}

//----------------------------------------------------------------------------
QWidget* svtkQtRecordView::GetWidget()
{
  return this->TextWidget;
}

//----------------------------------------------------------------------------
void svtkQtRecordView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  if (this->FieldType != type)
  {
    this->FieldType = type;
    this->Modified();
  }
}

void svtkQtRecordView::AddRepresentationInternal(svtkDataRepresentation* rep)
{
  svtkAlgorithmOutput* conn;
  conn = rep->GetInputConnection();

  this->DataObjectToTable->SetInputConnection(0, conn);
}

void svtkQtRecordView::RemoveRepresentationInternal(svtkDataRepresentation* rep)
{
  svtkAlgorithmOutput* conn;
  conn = rep->GetInputConnection();
  this->DataObjectToTable->RemoveInputConnection(0, conn);
}

//----------------------------------------------------------------------------
void svtkQtRecordView::Update()
{
  svtkDataRepresentation* rep = this->GetRepresentation();

  svtkAlgorithmOutput* conn = rep->GetInputConnection();
  svtkDataObject* d = conn->GetProducer()->GetOutputDataObject(0);
  svtkSelection* s = rep->GetAnnotationLink()->GetCurrentSelection();
  if (d->GetMTime() == this->LastInputMTime && this->LastMTime == this->GetMTime() &&
    s->GetMTime() == this->CurrentSelectionMTime)
  {
    return;
  }

  this->LastInputMTime = d->GetMTime();
  this->LastMTime = this->GetMTime();
  this->CurrentSelectionMTime = s->GetMTime();

  svtkStdString html;
  if (!rep)
  {
    this->TextWidget->setHtml(html.c_str());
    return;
  }

  this->DataObjectToTable->Update();
  svtkTable* table = this->DataObjectToTable->GetOutput();
  if (!table)
  {
    this->TextWidget->setHtml(html.c_str());
    return;
  }

  svtkSmartPointer<svtkSelection> cs;
  cs.TakeReference(
    svtkConvertSelection::ToSelectionType(rep->GetAnnotationLink()->GetCurrentSelection(), table,
      svtkSelectionNode::INDICES, nullptr, svtkSelectionNode::ROW));
  svtkSelectionNode* node = cs->GetNode(0);
  const svtkIdType column_count = table->GetNumberOfColumns();

  if (node)
  {
    svtkAbstractArray* indexArr = node->GetSelectionList();
    int numRecords = indexArr->GetNumberOfTuples() > 2 ? 2 : indexArr->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numRecords; ++i)
    {
      svtkVariant v(0);
      switch (indexArr->GetDataType())
      {
        svtkExtraExtendedTemplateMacro(v = *static_cast<SVTK_TT*>(indexArr->GetVoidPointer(i)));
      }

      for (svtkIdType j = 0; j != column_count; ++j)
      {
        html += "<b>";
        html += table->GetColumnName(j);
        html += ":</b> ";
        html += table->GetValue(v.ToInt(), j).ToString().c_str();
        html += "<br>\n";
      }
      html += "<br>\n<br>\n<br>\n<br>\n<br>\n";
    }
  }

  this->TextWidget->setHtml(html.c_str());
}

//----------------------------------------------------------------------------
void svtkQtRecordView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
