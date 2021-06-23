/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVertexDegree.cxx

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
#include "svtkVertexDegree.h"

#include "svtkCommand.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkVertexDegree);

svtkVertexDegree::svtkVertexDegree()
{
  this->OutputArrayName = nullptr;
}

svtkVertexDegree::~svtkVertexDegree()
{
  // release mem
  this->SetOutputArrayName(nullptr);
}

int svtkVertexDegree::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Do a shallow copy of the input to the output
  output->ShallowCopy(input);

  // Create the attribute array
  svtkIntArray* DegreeArray = svtkIntArray::New();
  if (this->OutputArrayName)
  {
    DegreeArray->SetName(this->OutputArrayName);
  }
  else
  {
    DegreeArray->SetName("VertexDegree");
  }
  DegreeArray->SetNumberOfTuples(output->GetNumberOfVertices());

  // Now loop through the vertices and set their degree in the array
  for (int i = 0; i < DegreeArray->GetNumberOfTuples(); ++i)
  {
    DegreeArray->SetValue(i, output->GetDegree(i));

    double progress =
      static_cast<double>(i) / static_cast<double>(DegreeArray->GetNumberOfTuples());
    this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
  }

  // Add attribute array to the output
  output->GetVertexData()->AddArray(DegreeArray);
  DegreeArray->Delete();

  return 1;
}

void svtkVertexDegree::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(none)")
     << endl;
}
