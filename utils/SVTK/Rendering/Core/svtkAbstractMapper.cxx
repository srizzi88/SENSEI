/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractMapper.h"

#include "svtkAbstractArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPlaneCollection.h"
#include "svtkPlanes.h"
#include "svtkPointData.h"
#include "svtkTimerLog.h"

svtkCxxSetObjectMacro(svtkAbstractMapper, ClippingPlanes, svtkPlaneCollection);

// Construct object.
svtkAbstractMapper::svtkAbstractMapper()
{
  this->TimeToDraw = 0.0;
  this->LastWindow = nullptr;
  this->ClippingPlanes = nullptr;
  this->Timer = svtkTimerLog::New();
  this->SetNumberOfOutputPorts(0);
  this->SetNumberOfInputPorts(1);
}

svtkAbstractMapper::~svtkAbstractMapper()
{
  this->Timer->Delete();
  if (this->ClippingPlanes)
  {
    this->ClippingPlanes->UnRegister(this);
  }
}

// Description:
// Override Modifiedtime as we have added Clipping planes
svtkMTimeType svtkAbstractMapper::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType clipMTime;

  if (this->ClippingPlanes != nullptr)
  {
    clipMTime = this->ClippingPlanes->GetMTime();
    mTime = (clipMTime > mTime ? clipMTime : mTime);
  }

  return mTime;
}

void svtkAbstractMapper::AddClippingPlane(svtkPlane* plane)
{
  if (this->ClippingPlanes == nullptr)
  {
    this->ClippingPlanes = svtkPlaneCollection::New();
    this->ClippingPlanes->Register(this);
    this->ClippingPlanes->Delete();
  }

  this->ClippingPlanes->AddItem(plane);
  this->Modified();
}

void svtkAbstractMapper::RemoveClippingPlane(svtkPlane* plane)
{
  if (this->ClippingPlanes == nullptr)
  {
    svtkErrorMacro(<< "Cannot remove clipping plane: mapper has none");
    return;
  }
  this->ClippingPlanes->RemoveItem(plane);
  this->Modified();
}

void svtkAbstractMapper::RemoveAllClippingPlanes()
{
  if (this->ClippingPlanes)
  {
    this->ClippingPlanes->RemoveAllItems();
  }
}

void svtkAbstractMapper::SetClippingPlanes(svtkPlanes* planes)
{
  svtkPlane* plane;
  if (!planes)
  {
    return;
  }

  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllClippingPlanes();
  for (int i = 0; i < numPlanes && i < 6; i++)
  {
    plane = svtkPlane::New();
    planes->GetPlane(i, plane);
    this->AddClippingPlane(plane);
    plane->Delete();
  }
}

svtkDataArray* svtkAbstractMapper::GetScalars(svtkDataSet* input, int scalarMode, int arrayAccessMode,
  int arrayId, const char* arrayName, int& cellFlag)
{
  svtkAbstractArray* abstractScalars = svtkAbstractMapper::GetAbstractScalars(
    input, scalarMode, arrayAccessMode, arrayId, arrayName, cellFlag);
  svtkDataArray* scalars = svtkArrayDownCast<svtkDataArray>(abstractScalars);
  return scalars;
}

svtkAbstractArray* svtkAbstractMapper::GetAbstractScalars(svtkDataSet* input, int scalarMode,
  int arrayAccessMode, int arrayId, const char* arrayName, int& cellFlag)
{
  svtkAbstractArray* scalars = nullptr;
  svtkPointData* pd;
  svtkCellData* cd;
  svtkFieldData* fd;

  // make sure we have an input
  if (!input)
  {
    return nullptr;
  }

  // get and scalar data according to scalar mode
  if (scalarMode == SVTK_SCALAR_MODE_DEFAULT)
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    if (!scalars)
    {
      scalars = input->GetCellData()->GetScalars();
      cellFlag = 1;
    }
  }
  else if (scalarMode == SVTK_SCALAR_MODE_USE_POINT_DATA)
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
  }
  else if (scalarMode == SVTK_SCALAR_MODE_USE_CELL_DATA)
  {
    scalars = input->GetCellData()->GetScalars();
    cellFlag = 1;
  }
  else if (scalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    pd = input->GetPointData();
    if (arrayAccessMode == SVTK_GET_ARRAY_BY_ID)
    {
      scalars = pd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = pd->GetAbstractArray(arrayName);
    }
    cellFlag = 0;
  }
  else if (scalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    cd = input->GetCellData();
    if (arrayAccessMode == SVTK_GET_ARRAY_BY_ID)
    {
      scalars = cd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = cd->GetAbstractArray(arrayName);
    }
    cellFlag = 1;
  }
  else if (scalarMode == SVTK_SCALAR_MODE_USE_FIELD_DATA)
  {
    fd = input->GetFieldData();
    if (arrayAccessMode == SVTK_GET_ARRAY_BY_ID)
    {
      scalars = fd->GetAbstractArray(arrayId);
    }
    else
    {
      scalars = fd->GetAbstractArray(arrayName);
    }
    cellFlag = 2;
  }

  return scalars;
}

// Shallow copy of svtkProp.
void svtkAbstractMapper::ShallowCopy(svtkAbstractMapper* mapper)
{
  this->SetClippingPlanes(mapper->GetClippingPlanes());
}

void svtkAbstractMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TimeToDraw: " << this->TimeToDraw << "\n";

  if (this->ClippingPlanes)
  {
    os << indent << "ClippingPlanes:\n";
    this->ClippingPlanes->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ClippingPlanes: (none)\n";
  }
}

int svtkAbstractMapper::GetNumberOfClippingPlanes()
{
  int n = 0;

  if (this->ClippingPlanes)
  {
    n = this->ClippingPlanes->GetNumberOfItems();
  }

  return n;
}
