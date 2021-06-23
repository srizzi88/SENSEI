/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssignCoordinatesLayoutStrategy.cxx

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

#include "svtkAssignCoordinatesLayoutStrategy.h"

#include "svtkAssignCoordinates.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkAssignCoordinatesLayoutStrategy);

svtkAssignCoordinatesLayoutStrategy::svtkAssignCoordinatesLayoutStrategy()
{
  this->AssignCoordinates = svtkSmartPointer<svtkAssignCoordinates>::New();
}

svtkAssignCoordinatesLayoutStrategy::~svtkAssignCoordinatesLayoutStrategy() = default;

void svtkAssignCoordinatesLayoutStrategy::SetXCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetXCoordArrayName(name);
}

const char* svtkAssignCoordinatesLayoutStrategy::GetXCoordArrayName()
{
  return this->AssignCoordinates->GetXCoordArrayName();
}

void svtkAssignCoordinatesLayoutStrategy::SetYCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetYCoordArrayName(name);
}

const char* svtkAssignCoordinatesLayoutStrategy::GetYCoordArrayName()
{
  return this->AssignCoordinates->GetYCoordArrayName();
}

void svtkAssignCoordinatesLayoutStrategy::SetZCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetZCoordArrayName(name);
}

const char* svtkAssignCoordinatesLayoutStrategy::GetZCoordArrayName()
{
  return this->AssignCoordinates->GetZCoordArrayName();
}

void svtkAssignCoordinatesLayoutStrategy::Layout()
{
  this->AssignCoordinates->SetInputData(this->Graph);
  this->AssignCoordinates->Update();
  this->Graph->ShallowCopy(this->AssignCoordinates->GetOutput());
}

void svtkAssignCoordinatesLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
