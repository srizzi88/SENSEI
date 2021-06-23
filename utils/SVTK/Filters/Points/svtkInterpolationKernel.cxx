/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInterpolationKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInterpolationKernel.h"
#include "svtkAbstractPointLocator.h"
#include "svtkDataSet.h"
#include "svtkPointData.h"

//----------------------------------------------------------------------------
svtkInterpolationKernel::svtkInterpolationKernel()
{
  this->RequiresInitialization = true;

  this->Locator = nullptr;
  this->DataSet = nullptr;
  this->PointData = nullptr;
}

//----------------------------------------------------------------------------
svtkInterpolationKernel::~svtkInterpolationKernel()
{
  this->FreeStructures();
}

//----------------------------------------------------------------------------
void svtkInterpolationKernel::FreeStructures()
{
  if (this->Locator)
  {
    this->Locator->Delete();
    this->Locator = nullptr;
  }

  if (this->DataSet)
  {
    this->DataSet->Delete();
    this->DataSet = nullptr;
  }

  if (this->PointData)
  {
    this->PointData->Delete();
    this->PointData = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkInterpolationKernel::Initialize(
  svtkAbstractPointLocator* loc, svtkDataSet* ds, svtkPointData* attr)
{
  this->FreeStructures();

  if (loc)
  {
    this->Locator = loc;
    this->Locator->Register(this);
  }

  if (ds)
  {
    this->DataSet = ds;
    this->DataSet->Register(this);
  }

  if (attr)
  {
    this->PointData = attr;
    this->PointData->Register(this);
  }
}

//----------------------------------------------------------------------------
void svtkInterpolationKernel::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Requires Initialization: " << (this->GetRequiresInitialization() ? "On\n" : "Off\n");

  if (this->Locator)
  {
    os << indent << "Locator:\n";
    this->Locator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: (None)\n";
  }

  if (this->DataSet)
  {
    os << indent << "DataSet:\n";
    this->DataSet->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "DataSet: (None)\n";
  }

  if (this->PointData)
  {
    os << indent << "PointData:\n";
    this->PointData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "PointData: (None)\n";
  }
}
