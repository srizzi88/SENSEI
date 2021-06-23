/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContourLineInterpolator.h"

#include "svtkContourRepresentation.h"
#include "svtkIntArray.h"

//----------------------------------------------------------------------
svtkContourLineInterpolator::svtkContourLineInterpolator() = default;

//----------------------------------------------------------------------
svtkContourLineInterpolator::~svtkContourLineInterpolator() = default;

//----------------------------------------------------------------------
int svtkContourLineInterpolator::UpdateNode(
  svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node), int svtkNotUsed(idx))
{
  return 0;
}

//----------------------------------------------------------------------
void svtkContourLineInterpolator::GetSpan(
  int nodeIndex, svtkIntArray* nodeIndices, svtkContourRepresentation* rep)
{
  int start = nodeIndex - 1;
  int end = nodeIndex;
  int index[2];

  // Clear the array
  nodeIndices->Reset();
  nodeIndices->Squeeze();
  nodeIndices->SetNumberOfComponents(2);

  for (int i = 0; i < 3; i++)
  {
    index[0] = start++;
    index[1] = end++;

    if (rep->GetClosedLoop())
    {
      if (index[0] < 0)
      {
        index[0] += rep->GetNumberOfNodes();
      }
      if (index[1] < 0)
      {
        index[1] += rep->GetNumberOfNodes();
      }
      if (index[0] >= rep->GetNumberOfNodes())
      {
        index[0] -= rep->GetNumberOfNodes();
      }
      if (index[1] >= rep->GetNumberOfNodes())
      {
        index[1] -= rep->GetNumberOfNodes();
      }
    }

    if (index[0] >= 0 && index[0] < rep->GetNumberOfNodes() && index[1] >= 0 &&
      index[1] < rep->GetNumberOfNodes())
    {
      nodeIndices->InsertNextTypedTuple(index);
    }
  }
}

//----------------------------------------------------------------------
void svtkContourLineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
