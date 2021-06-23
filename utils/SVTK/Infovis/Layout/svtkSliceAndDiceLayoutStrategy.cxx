/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliceAndDiceLayoutStrategy.cxx

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

#include "svtkSliceAndDiceLayoutStrategy.h"

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

svtkStandardNewMacro(svtkSliceAndDiceLayoutStrategy);

svtkSliceAndDiceLayoutStrategy::svtkSliceAndDiceLayoutStrategy() = default;

svtkSliceAndDiceLayoutStrategy::~svtkSliceAndDiceLayoutStrategy() = default;

void svtkSliceAndDiceLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// Alternating tree layout method
void svtkSliceAndDiceLayoutStrategy::Layout(
  svtkTree* inputTree, svtkDataArray* coordsArray, svtkDataArray* sizeArray)
{
  if (!inputTree)
  {
    return;
  }
  if (!coordsArray)
  {
    svtkErrorMacro("Area array undefined.");
    return;
  }

  svtkSmartPointer<svtkTreeDFSIterator> dfs = svtkSmartPointer<svtkTreeDFSIterator>::New();
  dfs->SetTree(inputTree);
  float coords[4];

  svtkSmartPointer<svtkAdjacentVertexIterator> it = svtkSmartPointer<svtkAdjacentVertexIterator>::New();

  while (dfs->HasNext())
  {
    svtkIdType vertex = dfs->Next();
    bool vertical = (inputTree->GetLevel(vertex) % 2) == 1;
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

    inputTree->GetChildren(vertex, it);
    float total = 0;
    while (it->HasNext())
    {
      if (sizeArray)
      {
        total += static_cast<float>(sizeArray->GetTuple1(it->Next()));
      }
      else
      {
        total += 1.0f;
      }
    }

    inputTree->GetChildren(vertex, it);
    // Give children their positions
    float part = 0;
    float oldDelta = 0;
    float delta = 0;
    while (it->HasNext())
    {
      svtkIdType child = it->Next();
      if (sizeArray)
      {
        part += static_cast<float>(sizeArray->GetTuple1(child));
      }
      else
      {
        part += 1.0f;
      }
      oldDelta = delta;
      if (vertical)
      {
        delta = xSpace * (part / total);
        coords[0] = parentMinX + oldDelta; // minX
        coords[1] = parentMinX + delta;    // maxX
        coords[2] = parentMinY;            // minY
        coords[3] = parentMaxY;            // maxY
      }
      else
      {
        delta = ySpace * (part / total);
        coords[0] = parentMinX;            // minX
        coords[1] = parentMaxX;            // maxX
        coords[2] = parentMaxY - delta;    // maxY
        coords[3] = parentMaxY - oldDelta; // minY
      }
      coordsArray->SetTuple(child, coords);
      inputTree->GetPoints()->SetPoint(
        child, (coords[0] + coords[1]) / 2.0, (coords[2] + coords[3]) / 2.0, 0.0);
    }
  }
}
