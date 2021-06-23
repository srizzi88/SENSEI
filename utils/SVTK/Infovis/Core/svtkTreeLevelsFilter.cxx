/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeLevelsFilter.cxx

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

#include "svtkTreeLevelsFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include "svtkGraph.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkTreeLevelsFilter);

svtkTreeLevelsFilter::svtkTreeLevelsFilter() = default;

int svtkTreeLevelsFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTree and outputTree handles
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* outputTree = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Copy the input to the output.
  outputTree->ShallowCopy(inputTree);

  // Add the 1-tuple array that will store the level from
  // the root down (root = 0, and +1 for each level down)
  svtkIntArray* levelArray = svtkIntArray::New();
  levelArray->SetName("level");
  levelArray->SetNumberOfComponents(1);
  levelArray->SetNumberOfTuples(outputTree->GetNumberOfVertices());
  svtkDataSetAttributes* data = outputTree->GetVertexData();
  data->AddArray(levelArray);

  // Add the 1-tuple array that will marks each
  // leaf with a '1' and everything else with a '0'
  svtkIntArray* leafArray = svtkIntArray::New();
  leafArray->SetName("leaf");
  leafArray->SetNumberOfComponents(1);
  leafArray->SetNumberOfTuples(outputTree->GetNumberOfVertices());
  data->AddArray(leafArray);

  for (svtkIdType i = 0; i < outputTree->GetNumberOfVertices(); i++)
  {
    levelArray->SetValue(i, outputTree->GetLevel(i));
    leafArray->SetValue(i, outputTree->IsLeaf(i));
  }

  // Set levels as the active point scalar
  data->SetActiveScalars("level");

  // Clean up
  levelArray->Delete();
  leafArray->Delete();

  return 1;
}

void svtkTreeLevelsFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
