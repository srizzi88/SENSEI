/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMapperNode.h"

#include "svtkAbstractArray.h"
#include "svtkAbstractVolumeMapper.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkProperty.h"

//============================================================================
svtkStandardNewMacro(svtkMapperNode);

//----------------------------------------------------------------------------
svtkMapperNode::svtkMapperNode() {}

//----------------------------------------------------------------------------
svtkMapperNode::~svtkMapperNode() {}

//----------------------------------------------------------------------------
void svtkMapperNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkMapperNode::GetArrayToProcess(svtkDataSet* input, int& cellFlag)
{
  cellFlag = -1;
  svtkAbstractVolumeMapper* mapper = svtkAbstractVolumeMapper::SafeDownCast(this->GetRenderable());
  if (!mapper)
  {
    return nullptr;
  }

  svtkAbstractArray* scalars;
  int scalarMode = mapper->GetScalarMode();
  if (scalarMode == SVTK_SCALAR_MODE_DEFAULT)
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    if (!scalars)
    {
      scalars = input->GetCellData()->GetScalars();
      cellFlag = 1;
    }
    return scalars;
  }

  if (scalarMode == SVTK_SCALAR_MODE_USE_POINT_DATA)
  {
    scalars = input->GetPointData()->GetScalars();
    cellFlag = 0;
    return scalars;
  }
  if (scalarMode == SVTK_SCALAR_MODE_USE_CELL_DATA)
  {
    scalars = input->GetCellData()->GetScalars();
    cellFlag = 1;
    return scalars;
  }

  int arrayAccessMode = mapper->GetArrayAccessMode();
  const char* arrayName = mapper->GetArrayName();
  int arrayId = mapper->GetArrayId();
  svtkPointData* pd;
  svtkCellData* cd;
  svtkFieldData* fd;
  if (scalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
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
    return scalars;
  }

  if (scalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
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
    return scalars;
  }

  if (scalarMode == SVTK_SCALAR_MODE_USE_FIELD_DATA)
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
    return scalars;
  }

  return nullptr;
}
