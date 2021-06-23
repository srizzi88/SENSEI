/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractSelectedArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPExtractSelectedArraysOverTime.h"

#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPExtractDataArraysOverTime.h"

svtkStandardNewMacro(svtkPExtractSelectedArraysOverTime);
//----------------------------------------------------------------------------
svtkPExtractSelectedArraysOverTime::svtkPExtractSelectedArraysOverTime()
{
  this->ArraysExtractor = svtkSmartPointer<svtkPExtractDataArraysOverTime>::New();
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkPExtractSelectedArraysOverTime::~svtkPExtractSelectedArraysOverTime()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void svtkPExtractSelectedArraysOverTime::SetController(svtkMultiProcessController* controller)
{
  auto extractor = svtkPExtractDataArraysOverTime::SafeDownCast(this->ArraysExtractor);
  if (extractor && extractor->GetController() != controller)
  {
    extractor->SetController(controller);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkMultiProcessController* svtkPExtractSelectedArraysOverTime::GetController()
{
  auto extractor = svtkPExtractDataArraysOverTime::SafeDownCast(this->ArraysExtractor);
  return (extractor ? extractor->GetController() : nullptr);
}

//----------------------------------------------------------------------------
void svtkPExtractSelectedArraysOverTime::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->GetController() << endl;
}
