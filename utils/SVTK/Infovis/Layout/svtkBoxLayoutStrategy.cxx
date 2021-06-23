/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxLayoutStrategy.cxx

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

#include "svtkBoxLayoutStrategy.h"

#include "svtkAdjacentVertexIterator.h"
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
#include "svtkSmartPointer.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"

svtkStandardNewMacro(svtkBoxLayoutStrategy);

svtkBoxLayoutStrategy::svtkBoxLayoutStrategy() = default;

svtkBoxLayoutStrategy::~svtkBoxLayoutStrategy() = default;

void svtkBoxLayoutStrategy::Layout(
  svtkTree* inputTree, svtkDataArray* coordsArray, svtkDataArray* svtkNotUsed(sizeArray))
{
  if (!inputTree)
  {
    return;
  }
  if (!coordsArray)
  {
    svtkErrorMacro("Area array not defined.");
    return;
  }
  svtkSmartPointer<svtkTreeDFSIterator> dfs = svtkSmartPointer<svtkTreeDFSIterator>::New();
  dfs->SetTree(inputTree);
  float coords[4];
  svtkSmartPointer<svtkAdjacentVertexIterator> children =
    svtkSmartPointer<svtkAdjacentVertexIterator>::New();
  while (dfs->HasNext())
  {
    svtkIdType vertex = dfs->Next();
    if (vertex == inputTree->GetRoot())
    {
      coords[0] = 0;
      coords[1] = 1;
      coords[2] = 0;
      coords[3] = 1;
      coordsArray->SetTuple(vertex, coords);
      inputTree->GetPoints()->SetPoint(
        vertex, (coords[0] + coords[1]) / 2.0, (coords[2] + coords[3]) / 2.0, 0.0);
    }
    double doubleCoords[4];
    coordsArray->GetTuple(vertex, doubleCoords);
    for (int i = 0; i < 4; i++)
    {
      coords[i] = doubleCoords[i];
    }
    this->AddBorder(coords);
    float parentMinX = coords[0];
    float parentMaxX = coords[1];
    float parentMinY = coords[2];
    float parentMaxY = coords[3];
    float xSpace = parentMaxX - parentMinX;
    float ySpace = parentMaxY - parentMinY;

    svtkIdType nchildren = inputTree->GetNumberOfChildren(vertex);
    if (!inputTree->IsLeaf(vertex))
    {
      // Divide the available space with simple algo
      int xDivisions = static_cast<int>(sqrt(static_cast<double>(nchildren)) + 1); // Ceiling
      int yDivisions = xDivisions;

      // Okay try shrinking the bounds
      if ((xDivisions - 1) * yDivisions >= nchildren)
        --xDivisions;
      if (xDivisions * (yDivisions - 1) >= nchildren)
        --yDivisions;

      // Get the children
      inputTree->GetChildren(vertex, children);

      // Now break up the space evenly and pack
      float xDelta = xSpace / xDivisions;
      float yDelta = ySpace / yDivisions;
      for (int i = 0; i < yDivisions; i++)
      {
        for (int j = 0; j < xDivisions; j++)
        {
          // Check to see if we have more children
          if (!children->HasNext())
          {
            break;
          }
          svtkIdType child = children->Next();

          // Give children their positions
          coords[0] = parentMinX + xDelta * j;                  // minX
          coords[1] = parentMinX + xDelta * (j + 1.0);          // maxX
          coords[2] = parentMinY + ySpace - yDelta * (i + 1.0); // minY
          coords[3] = parentMinY + ySpace - yDelta * i;         // maxY

          coordsArray->SetTuple(child, coords);
          inputTree->GetPoints()->SetPoint(
            child, (coords[0] + coords[1]) / 2.0, (coords[2] + coords[3]) / 2.0, 0.0);
        }
      }
    }
  }
}

void svtkBoxLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
