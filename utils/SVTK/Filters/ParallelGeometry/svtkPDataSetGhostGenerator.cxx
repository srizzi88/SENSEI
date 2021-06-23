/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkPDataSetGhostGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkPDataSetGhostGenerator.h"

#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"

#include <cassert>

svtkPDataSetGhostGenerator::svtkPDataSetGhostGenerator()
{
  this->Initialized = false;
  this->Controller = svtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
svtkPDataSetGhostGenerator::~svtkPDataSetGhostGenerator() {}

//------------------------------------------------------------------------------
void svtkPDataSetGhostGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Controller: " << this->Controller << std::endl;
}

//------------------------------------------------------------------------------
void svtkPDataSetGhostGenerator::Initialize()
{
  assert("pre: Multi-process controller is nullptr" && (this->Controller != nullptr));
  this->Rank = this->Controller->GetLocalProcessId();
  this->Initialized = true;
}

//------------------------------------------------------------------------------
void svtkPDataSetGhostGenerator::Barrier()
{
  assert("pre: Multi-process controller is nullptr" && (this->Controller != nullptr));
  assert("pre: Instance has not been initialized!" && this->Initialized);
  this->Controller->Barrier();
}
