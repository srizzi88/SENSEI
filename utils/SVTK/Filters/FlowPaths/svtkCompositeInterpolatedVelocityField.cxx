/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeInterpolatedVelocityField.h"

#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkGenericCell.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

//----------------------------------------------------------------------------
svtkCompositeInterpolatedVelocityField::svtkCompositeInterpolatedVelocityField()
{
  this->LastDataSetIndex = 0;
  this->DataSets = new svtkCompositeInterpolatedVelocityFieldDataSetsType;
}

//----------------------------------------------------------------------------
svtkCompositeInterpolatedVelocityField::~svtkCompositeInterpolatedVelocityField()
{
  delete this->DataSets;
  this->DataSets = nullptr;
}

//----------------------------------------------------------------------------
void svtkCompositeInterpolatedVelocityField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataSets: " << this->DataSets << endl;
  os << indent << "Last Dataset Index: " << this->LastDataSetIndex << endl;
}
