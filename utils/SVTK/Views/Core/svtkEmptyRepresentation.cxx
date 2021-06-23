/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEmptyRepresentation.cxx

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

#include "svtkEmptyRepresentation.h"

#include "svtkAnnotationLink.h"
#include "svtkConvertSelectionDomain.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkEmptyRepresentation);

svtkEmptyRepresentation::svtkEmptyRepresentation()
{
  this->ConvertDomains = svtkSmartPointer<svtkConvertSelectionDomain>::New();

  this->SetNumberOfInputPorts(0);
}

svtkEmptyRepresentation::~svtkEmptyRepresentation() = default;

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkEmptyRepresentation::GetInternalAnnotationOutputPort(
  int svtkNotUsed(port), int svtkNotUsed(conn))
{
  this->ConvertDomains->SetInputConnection(0, this->GetAnnotationLink()->GetOutputPort(0));
  this->ConvertDomains->SetInputConnection(1, this->GetAnnotationLink()->GetOutputPort(1));

  return this->ConvertDomains->GetOutputPort();
}

void svtkEmptyRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
