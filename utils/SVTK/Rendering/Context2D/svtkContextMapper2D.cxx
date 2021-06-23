/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextMapper2D.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkTable.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkContextMapper2D);
//-----------------------------------------------------------------------------
svtkContextMapper2D::svtkContextMapper2D()
{
  // We take 1 input and no outputs
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//-----------------------------------------------------------------------------
svtkContextMapper2D::~svtkContextMapper2D() = default;

//----------------------------------------------------------------------------
void svtkContextMapper2D::SetInputData(svtkTable* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
svtkTable* svtkContextMapper2D::GetInput()
{
  return svtkTable::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//-----------------------------------------------------------------------------
int svtkContextMapper2D::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkContextMapper2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
