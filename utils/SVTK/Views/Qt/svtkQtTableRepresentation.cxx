/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTableRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkQtTableRepresentation.h"
#include "svtkQtTableModelAdapter.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"

#include <QColor>
#include <QModelIndex>

#include <cassert>

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkQtTableRepresentation, ColorTable, svtkLookupTable);

// ----------------------------------------------------------------------
svtkQtTableRepresentation::svtkQtTableRepresentation()
{
  this->ModelAdapter = new svtkQtTableModelAdapter;

  this->ColorTable = svtkLookupTable::New();
  this->ColorTable->Register(this);
  this->ColorTable->Delete();

  this->ColorTable->SetHueRange(0.0, 1.0);
  this->ColorTable->SetRange(0.0, 1.0);
  this->ColorTable->Build();

  this->SeriesColors = svtkDoubleArray::New();
  this->SeriesColors->SetNumberOfComponents(4);
  this->SeriesColors->Register(this);
  this->SeriesColors->Delete();

  this->KeyColumnInternal = nullptr;
  this->FirstDataColumn = nullptr;
  this->LastDataColumn = nullptr;
}

// ----------------------------------------------------------------------

svtkQtTableRepresentation::~svtkQtTableRepresentation()
{
  delete this->ModelAdapter;
  this->ColorTable->UnRegister(this);
  this->SeriesColors->UnRegister(this);
  this->SetKeyColumnInternal(nullptr);
  this->SetFirstDataColumn(nullptr);
  this->SetLastDataColumn(nullptr);
}

// ----------------------------------------------------------------------

int svtkQtTableRepresentation::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  this->UpdateTable();
  return 1;
}

// ----------------------------------------------------------------------

void svtkQtTableRepresentation::SetKeyColumn(const char* col)
{
  if ((!col && !this->KeyColumnInternal) ||
    (this->KeyColumnInternal && col && strcmp(this->KeyColumnInternal, col) == 0))
  {
    return;
  }

  this->SetKeyColumnInternal(col);
  this->ModelAdapter->SetKeyColumn(-1);
  this->Modified();
  // We don't call Update(), representations should not call Update() on
  // themselves when their ivars are changed. It's almost like a svtkAlgorithm
  // calling Update() on itself when an ivar change which is not recommended.
  // this->Update();
}

// ----------------------------------------------------------------------

char* svtkQtTableRepresentation::GetKeyColumn()
{
  return this->GetKeyColumnInternal();
}

// ----------------------------------------------------------------------
void svtkQtTableRepresentation::UpdateTable()
{
  this->ResetModel();

  if (!this->GetInput())
  {
    return;
  }

  svtkTable* table = svtkTable::SafeDownCast(this->GetInput());
  if (!table)
  {
    svtkErrorMacro(<< "svtkQtTableRepresentation: I need a svtkTable as input.  You supplied a "
                  << this->GetInput()->GetClassName() << ".");
    return;
  }

  // Set first/last data column names if they
  // have not already been set.
  const char* firstDataColumn = this->FirstDataColumn;
  const char* lastDataColumn = this->LastDataColumn;
  if (!firstDataColumn)
  {
    firstDataColumn = table->GetColumnName(0);
  }
  if (!lastDataColumn)
  {
    lastDataColumn = table->GetColumnName(table->GetNumberOfColumns() - 1);
  }

  // Now that we're sure of having data, put it into a Qt model
  // adapter that we can push into the QListView.  Before we hand that
  // off, though, we'll need to come up with colors for
  // each series.
  // int keyColumnIndex = -1;
  int firstDataColumnIndex = -1;
  int lastDataColumnIndex = -1;
  // if (this->KeyColumnInternal != nullptr)
  //  {
  //  table->GetRowData()->GetAbstractArray(this->KeyColumnInternal, keyColumnIndex);
  //  if (keyColumnIndex >= 0)
  //    {
  //    this->ModelAdapter->SetKeyColumn(keyColumnIndex);
  //    }
  //  else
  //    {
  //    // Either the user didn't specify a key column or else it wasn't
  //    // found.  We'll do the best we can.
  //    svtkWarningMacro(<<"svtkQtTableRepresentation: Key column "
  //                    << (this->KeyColumnInternal ? this->KeyColumnInternal : "(nullptr)")
  //                    << " not found.  Defaulting to column 0.");
  //    this->ModelAdapter->SetKeyColumn(0);
  //    }
  //  }
  if (firstDataColumn != nullptr)
  {
    table->GetRowData()->GetAbstractArray(firstDataColumn, firstDataColumnIndex);
  }
  if (lastDataColumn != nullptr)
  {
    table->GetRowData()->GetAbstractArray(lastDataColumn, lastDataColumnIndex);
  }
  this->ModelAdapter->SetDataColumnRange(firstDataColumnIndex, lastDataColumnIndex);

  // The view will try to do this when we add the representation, but
  // we need the model to be populated before that so we'll just do it
  // here.

  this->ModelAdapter->SetSVTKDataObject(table);
  if (this->KeyColumnInternal != nullptr)
  {
    this->ModelAdapter->SetKeyColumnName(this->KeyColumnInternal);
  }
  this->CreateSeriesColors();
}

// ----------------------------------------------------------------------

void svtkQtTableRepresentation::ResetModel()
{
  this->SetModelType();
  if (this->ModelAdapter)
  {
    // FIXME
    // Need to alert the model of potential changes to the svtkTable
    // in different way than disconnecting/reconnecting the svtkTable from
    // the model adapter
    // this->ModelAdapter->SetSVTKDataObject(nullptr);
  }
  this->SeriesColors->Reset();
  this->SeriesColors->SetNumberOfComponents(4);
}

// ----------------------------------------------------------------------

void svtkQtTableRepresentation::CreateSeriesColors()
{
  this->SeriesColors->Reset();
  this->SeriesColors->SetNumberOfComponents(4);

  int size = this->ModelAdapter->rowCount(QModelIndex());

  this->SeriesColors->SetNumberOfTuples(size);

  for (int i = 0; i < size; ++i)
  {
    double seriesValue = 1;
    if (size > 1)
    {
      seriesValue = static_cast<double>(i) / (size - 1);
    }
    QColor c;
    if (this->ColorTable)
    {
      double rgb[3];
      double opacity;
      this->ColorTable->GetColor(seriesValue, rgb);
      opacity = this->ColorTable->GetOpacity(seriesValue);
      c.setRgbF(rgb[0], rgb[1], rgb[2], opacity);
    }
    else
    {
      c.setHsvF(seriesValue, 1, 0.7);
    }

    this->SeriesColors->SetComponent(i, 0, c.redF());
    this->SeriesColors->SetComponent(i, 1, c.greenF());
    this->SeriesColors->SetComponent(i, 2, c.blueF());
    this->SeriesColors->SetComponent(i, 3, c.alphaF());
  }
}

// ----------------------------------------------------------------------

void svtkQtTableRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "First data column: " << (this->FirstDataColumn ? this->FirstDataColumn : "(nullptr)")
     << "\n";

  os << indent
     << "Last data column: " << (this->LastDataColumn ? this->LastDataColumn : "(nullptr)") << "\n";

  os << indent
     << "Key column: " << (this->KeyColumnInternal ? this->KeyColumnInternal : "(nullptr)") << "\n";

  os << indent << "Model adapter: Qt object " << this->ModelAdapter << "\n";

  os << indent << "Color creation table: ";
  this->ColorTable->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Series color table: ";
  this->SeriesColors->PrintSelf(os, indent.GetNextIndent());
}
