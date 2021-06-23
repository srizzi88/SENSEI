/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIElementBlockCellIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCPExodusIIElementBlockCellIterator.h"

#include "svtkCPExodusIIElementBlock.h"
#include "svtkCPExodusIIElementBlockPrivate.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"

#include <algorithm>

svtkStandardNewMacro(svtkCPExodusIIElementBlockCellIterator);

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Storage: " << this->Storage << endl;
  os << indent << "DataSetPoints: " << this->DataSetPoints << endl;
  os << indent << "CellId: " << this->CellId << endl;
}

//------------------------------------------------------------------------------
bool svtkCPExodusIIElementBlockCellIterator::IsValid()
{
  return this->Storage && this->CellId < this->Storage->NumberOfCells;
}

//------------------------------------------------------------------------------
svtkIdType svtkCPExodusIIElementBlockCellIterator::GetCellId()
{
  return this->CellId;
}

//------------------------------------------------------------------------------
svtkCPExodusIIElementBlockCellIterator::svtkCPExodusIIElementBlockCellIterator()
  : Storage(nullptr)
  , DataSetPoints(nullptr)
  , CellId(0)
{
}

//------------------------------------------------------------------------------
svtkCPExodusIIElementBlockCellIterator::~svtkCPExodusIIElementBlockCellIterator() {}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::ResetToFirstCell()
{
  this->CellId = 0;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::IncrementToNextCell()
{
  ++this->CellId;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::FetchCellType()
{
  this->CellType = this->Storage->CellType;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::FetchPointIds()
{
  this->PointIds->SetNumberOfIds(this->Storage->CellSize);

  std::transform(this->Storage->GetElementStart(this->CellId),
    this->Storage->GetElementEnd(this->CellId), this->PointIds->GetPointer(0),
    StorageType::NodeToPoint);
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::FetchPoints()
{
  this->DataSetPoints->GetPoints(this->GetPointIds(), this->Points);
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockCellIterator::SetStorage(svtkCPExodusIIElementBlock* eb)
{
  if (eb != nullptr)
  {
    this->Storage = eb->GetInternals();
    this->DataSetPoints = eb->GetPoints();
    if (this->DataSetPoints)
    {
      this->Points->SetDataType(this->DataSetPoints->GetDataType());
    }
  }
  else
  {
    this->Storage = nullptr;
    this->DataSetPoints = nullptr;
  }
  this->CellId = 0;
}
