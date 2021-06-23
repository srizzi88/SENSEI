/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSquarifyLayoutStrategy.cxx

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
#include "svtkSquarifyLayoutStrategy.h"

#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkSquarifyLayoutStrategy);

svtkSquarifyLayoutStrategy::svtkSquarifyLayoutStrategy() = default;

svtkSquarifyLayoutStrategy::~svtkSquarifyLayoutStrategy() = default;

void svtkSquarifyLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkSquarifyLayoutStrategy::Layout(
  svtkTree* inputTree, svtkDataArray* coordsArray, svtkDataArray* sizeArray)
{
  if (!inputTree || inputTree->GetNumberOfVertices() == 0)
  {
    return;
  }
  if (!coordsArray)
  {
    svtkErrorMacro("Area array undefined");
    return;
  }

  // Zero out coords and make points offscreen by default
  for (svtkIdType i = 0; i < inputTree->GetNumberOfVertices(); ++i)
  {
    coordsArray->SetTuple4(i, 0, 0, 0, 0);
    inputTree->GetPoints()->SetPoint(i, -100, -100, 0);
  }

  // Get the root vertex and set it to 0,1,0,1
  svtkIdType rootId = inputTree->GetRoot();
  float coords[] = { 0, 1, 0, 1 };
  coordsArray->SetTuple(rootId, coords);
  inputTree->GetPoints()->SetPoint(rootId, 0.5, 0.5, 0.0);

  // Now layout the children vertices
  this->AddBorder(coords);
  this->LayoutChildren(inputTree, coordsArray, sizeArray, inputTree->GetNumberOfChildren(rootId),
    rootId, 0, coords[0], coords[1], coords[2], coords[3]);
}

void svtkSquarifyLayoutStrategy::LayoutChildren(svtkTree* tree, svtkDataArray* coordsArray,
  svtkDataArray* sizeArray, svtkIdType nchildren, svtkIdType parent, svtkIdType begin, float minX,
  float maxX, float minY, float maxY)
{
  float sizeX = maxX - minX;
  float sizeY = maxY - minY;
  if ((sizeX == 0.0) || (sizeY == 0.0))
  {
    svtkErrorMacro(<< "Invalid Box Sizes for Vertex: " << tree->GetChild(parent, begin) << " ("
                  << sizeX << ", " << sizeY << ")");
    return;
  }
  bool vertical = (sizeX < sizeY);
  float total = 0;
  if (sizeArray)
  {
    for (svtkIdType i = begin; i < nchildren; i++)
    {
      total += static_cast<float>(sizeArray->GetTuple1(tree->GetChild(parent, i)));
    }
  }
  else
  {
    total = nchildren;
  }
  float factor = (sizeX * sizeY) / total;

  svtkIdType cur = begin;
  float oldRowError = SVTK_FLOAT_MAX;
  float rowError = SVTK_FLOAT_MAX;
  float oldCurTotal = 0;
  float curTotal = 0;
  while (rowError <= oldRowError && cur < nchildren)
  {
    oldCurTotal = curTotal;
    if (sizeArray)
    {
      curTotal += factor * static_cast<float>(sizeArray->GetTuple1(tree->GetChild(parent, cur)));
    }
    else
    {
      curTotal += 1.0f;
    }

    oldRowError = rowError;
    // Compute the new row error
    rowError = 0;
    float width = vertical ? (curTotal / sizeX) : (curTotal / sizeY);
    for (svtkIdType i = begin; i <= cur; i++)
    {
      float curValue = 1.0f;
      if (sizeArray)
      {
        curValue = static_cast<float>(sizeArray->GetTuple1(tree->GetChild(parent, i)));
      }
      float curHeight = factor * curValue / width;
      float ratio1 = curHeight / width;
      float ratio2 = width / curHeight;
      float curError = (ratio1 > ratio2) ? ratio1 : ratio2;
      if (curError > rowError)
      {
        rowError = curError;
      }
    }
    cur++;
  }
  if (rowError > oldRowError)
  {
    cur--;
    curTotal = oldCurTotal;
  }

  float coords[4];
  float rowMinX = minX;
  float rowMinY = minY;
  float rowMaxX;
  float rowMaxY = maxY;
  if (vertical)
  {
    rowMaxX = maxX;
    rowMinY = rowMaxY - curTotal / sizeX;
  }
  else
  {
    rowMaxX = rowMinX + curTotal / sizeY;
    rowMinY = minY;
  }
  float part = 0;
  float oldPosition = 0;
  float position = 0;
  for (svtkIdType j = begin; j < cur; j++)
  {
    int id = tree->GetChild(parent, j);
    if (sizeArray)
    {
      part += factor * static_cast<float>(sizeArray->GetTuple1(id));
    }
    else
    {
      part += factor * 1.0f;
    }
    oldPosition = position;
    // Give children their positions
    if (vertical)
    {
      if (curTotal == 0)
      {
        position = 0;
      }
      else
      {
        position = sizeX * (part / curTotal);
      }
      coords[0] = rowMinX + oldPosition; // minX
      coords[1] = rowMinX + position;    // maxX
      coords[2] = rowMinY;               // minY
      coords[3] = rowMaxY;               // maxY
    }
    else
    {
      if (curTotal == 0)
      {
        position = 0;
      }
      else
      {
        position = sizeY * (part / curTotal);
      }
      coords[0] = rowMinX;               // minX
      coords[1] = rowMaxX;               // maxX
      coords[2] = rowMaxY - position;    // minY
      coords[3] = rowMaxY - oldPosition; // maxY
    }

    coordsArray->SetTuple(id, coords);
    tree->GetPoints()->SetPoint(
      id, (coords[0] + coords[1]) / 2.0, (coords[2] + coords[3]) / 2.0, 0.0);
    svtkIdType numNewChildren = tree->GetNumberOfChildren(id);
    if (numNewChildren > 0)
    {
      this->AddBorder(coords);
      this->LayoutChildren(tree, coordsArray, sizeArray, numNewChildren, id, 0, coords[0],
        coords[1], coords[2], coords[3]);
    }
  }

  if (cur < nchildren)
  {
    float restMinX;
    float restMaxX;
    float restMinY;
    float restMaxY;
    if (vertical)
    {
      restMinY = minY;
      restMaxY = rowMinY;
      restMinX = rowMinX;
      restMaxX = rowMaxX;
    }
    else
    {
      restMinX = rowMaxX;
      restMaxX = maxX;
      restMinY = rowMinY;
      restMaxY = rowMaxY;
    }
    this->LayoutChildren(
      tree, coordsArray, sizeArray, nchildren, parent, cur, restMinX, restMaxX, restMinY, restMaxY);
  }
}
