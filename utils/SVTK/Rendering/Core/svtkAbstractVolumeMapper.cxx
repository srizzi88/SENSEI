/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractVolumeMapper.h"

#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkMath.h"

// Construct a svtkAbstractVolumeMapper
svtkAbstractVolumeMapper::svtkAbstractVolumeMapper()
{
  svtkMath::UninitializeBounds(this->Bounds);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->ScalarMode = SVTK_SCALAR_MODE_DEFAULT;

  this->ArrayName = new char[1];
  this->ArrayName[0] = '\0';
  this->ArrayId = -1;
  this->ArrayAccessMode = SVTK_GET_ARRAY_BY_ID;
}

svtkAbstractVolumeMapper::~svtkAbstractVolumeMapper()
{
  delete[] this->ArrayName;
}

// Get the bounds for the input of this mapper as
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* svtkAbstractVolumeMapper::GetBounds()
{
  if (!this->GetDataSetInput())
  {
    svtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->Update();
    this->GetDataSetInput()->GetBounds(this->Bounds);
    return this->Bounds;
  }
}

svtkDataObject* svtkAbstractVolumeMapper::GetDataObjectInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return this->GetInputDataObject(0, 0);
}

svtkDataSet* svtkAbstractVolumeMapper::GetDataSetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
int svtkAbstractVolumeMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

void svtkAbstractVolumeMapper::SelectScalarArray(int arrayNum)
{
  if ((this->ArrayId == arrayNum) && (this->ArrayAccessMode == SVTK_GET_ARRAY_BY_ID))
  {
    return;
  }
  this->Modified();

  this->ArrayId = arrayNum;
  this->ArrayAccessMode = SVTK_GET_ARRAY_BY_ID;
}

void svtkAbstractVolumeMapper::SelectScalarArray(const char* arrayName)
{
  if (!arrayName ||
    ((strcmp(this->ArrayName, arrayName) == 0) && (this->ArrayAccessMode == SVTK_GET_ARRAY_BY_NAME)))
  {
    return;
  }
  this->Modified();

  delete[] this->ArrayName;
  this->ArrayName = new char[strlen(arrayName) + 1];
  strcpy(this->ArrayName, arrayName);
  this->ArrayAccessMode = SVTK_GET_ARRAY_BY_NAME;
}

// Return the method for obtaining scalar data.
const char* svtkAbstractVolumeMapper::GetScalarModeAsString()
{
  if (this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_DATA)
  {
    return "UseCellData";
  }
  else if (this->ScalarMode == SVTK_SCALAR_MODE_USE_POINT_DATA)
  {
    return "UsePointData";
  }
  else if (this->ScalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    return "UsePointFieldData";
  }
  else if (this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    return "UseCellFieldData";
  }
  else
  {
    return "Default";
  }
}

// Print the svtkAbstractVolumeMapper
void svtkAbstractVolumeMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalarMode: " << this->GetScalarModeAsString() << endl;

  if (this->ScalarMode == SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
    this->ScalarMode == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    if (this->ArrayAccessMode == SVTK_GET_ARRAY_BY_ID)
    {
      os << indent << "ArrayId: " << this->ArrayId << endl;
    }
    else
    {
      os << indent << "ArrayName: " << this->ArrayName << endl;
    }
  }
}
