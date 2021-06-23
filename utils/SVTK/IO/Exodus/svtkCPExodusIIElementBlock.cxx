/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIElementBlock.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCPExodusIIElementBlock.h"

#include "svtkCellType.h"
#include "svtkCellTypes.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"

#include <algorithm>

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkCPExodusIIElementBlock);
svtkStandardNewMacro(svtkCPExodusIIElementBlockImpl);

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockImpl::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Elements: " << this->Elements << endl;
  os << indent << "CellType: " << svtkCellTypes::GetClassNameFromTypeId(this->CellType) << endl;
  os << indent << "CellSize: " << this->CellSize << endl;
  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
}

//------------------------------------------------------------------------------
bool svtkCPExodusIIElementBlockImpl::SetExodusConnectivityArray(
  int* elements, const std::string& type, int numElements, int nodesPerElement)
{
  if (!elements)
  {
    return false;
  }

  // Try to figure out the svtk cell type:
  if (type.size() < 3)
  {
    svtkErrorMacro(<< "Element type too short, expected at least 3 char: " << type);
    return false;
  }
  std::string typekey = type.substr(0, 3);
  std::transform(typekey.begin(), typekey.end(), typekey.begin(), ::toupper);
  if (typekey == "CIR" || typekey == "SPH")
  {
    this->CellType = SVTK_VERTEX;
  }
  else if (typekey == "TRU" || typekey == "BEA")
  {
    this->CellType = SVTK_LINE;
  }
  else if (typekey == "TRI")
  {
    this->CellType = SVTK_TRIANGLE;
  }
  else if (typekey == "QUA" || typekey == "SHE")
  {
    this->CellType = SVTK_QUAD;
  }
  else if (typekey == "TET")
  {
    this->CellType = SVTK_TETRA;
  }
  else if (typekey == "WED")
  {
    this->CellType = SVTK_WEDGE;
  }
  else if (typekey == "HEX")
  {
    this->CellType = SVTK_HEXAHEDRON;
  }
  else
  {
    svtkErrorMacro(<< "Unknown cell type: " << type);
    return false;
  }

  this->CellSize = static_cast<svtkIdType>(nodesPerElement);
  this->NumberOfCells = static_cast<svtkIdType>(numElements);
  this->Elements = elements;
  this->Modified();

  return true;
}

//------------------------------------------------------------------------------
svtkIdType svtkCPExodusIIElementBlockImpl::GetNumberOfCells()
{
  return this->NumberOfCells;
}

//------------------------------------------------------------------------------
int svtkCPExodusIIElementBlockImpl::GetCellType(svtkIdType)
{
  return this->CellType;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockImpl::GetCellPoints(svtkIdType cellId, svtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(this->CellSize);

  std::transform(
    this->GetElementStart(cellId), this->GetElementEnd(cellId), ptIds->GetPointer(0), NodeToPoint);
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockImpl::GetPointCells(svtkIdType ptId, svtkIdList* cellIds)
{
  const int targetElement = PointToNode(ptId);
  int* element = this->GetStart();
  int* elementEnd = this->GetEnd();

  cellIds->Reset();

  element = std::find(element, elementEnd, targetElement);
  while (element != elementEnd)
  {
    cellIds->InsertNextId(static_cast<svtkIdType>((element - this->Elements) / this->CellSize));
    element = std::find(element, elementEnd, targetElement);
  }
}

//------------------------------------------------------------------------------
int svtkCPExodusIIElementBlockImpl::GetMaxCellSize()
{
  return this->CellSize;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockImpl::GetIdsOfCellsOfType(int type, svtkIdTypeArray* array)
{
  array->Reset();
  if (type == this->CellType)
  {
    array->SetNumberOfComponents(1);
    array->Allocate(this->NumberOfCells);
    for (svtkIdType i = 0; i < this->NumberOfCells; ++i)
    {
      array->InsertNextValue(i);
    }
  }
}

//------------------------------------------------------------------------------
int svtkCPExodusIIElementBlockImpl::IsHomogeneous()
{
  return 1;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockImpl::Allocate(svtkIdType, int)
{
  svtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
svtkIdType svtkCPExodusIIElementBlockImpl::InsertNextCell(int, svtkIdList*)
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
svtkIdType svtkCPExodusIIElementBlockImpl::InsertNextCell(int, svtkIdType, const svtkIdType[])
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
svtkIdType svtkCPExodusIIElementBlockImpl::InsertNextCell(
  int, svtkIdType, const svtkIdType[], svtkIdType, const svtkIdType[])
{
  svtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
void svtkCPExodusIIElementBlockImpl::ReplaceCell(svtkIdType, int, const svtkIdType[])
{
  svtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
svtkCPExodusIIElementBlockImpl::svtkCPExodusIIElementBlockImpl()
  : Elements(nullptr)
  , CellType(SVTK_EMPTY_CELL)
  , CellSize(0)
  , NumberOfCells(0)
{
}

//------------------------------------------------------------------------------
svtkCPExodusIIElementBlockImpl::~svtkCPExodusIIElementBlockImpl()
{
  delete[] this->Elements;
}
