/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRandomGraphSource.cxx

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
#include "svtkAdjacentVertexIterator.h"
#include "svtkBitArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkRandomGraphSource.h"
#include "svtkSmartPointer.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestRandomGraphSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  SVTK_CREATE(svtkRandomGraphSource, source);

  int errors = 0;

  cerr << "Testing simple generator..." << endl;
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(200);
  source->Update();
  svtkGraph* g = source->GetOutput();
  if (g->GetNumberOfVertices() != 100)
  {
    cerr << "ERROR: Wrong number of vertices (" << g->GetNumberOfVertices() << " != " << 100 << ")"
         << endl;
    errors++;
  }
  if (g->GetNumberOfEdges() != 200)
  {
    cerr << "ERROR: Wrong number of edges (" << g->GetNumberOfEdges() << " != " << 200 << ")"
         << endl;
    errors++;
  }
  cerr << "...done." << endl;

  cerr << "Testing simple generator..." << endl;
  source->SetStartWithTree(true);
  source->Update();
  g = source->GetOutput();
  if (g->GetNumberOfVertices() != 100)
  {
    cerr << "ERROR: Wrong number of vertices (" << g->GetNumberOfVertices() << " != " << 100 << ")"
         << endl;
    errors++;
  }
  if (g->GetNumberOfEdges() != 299)
  {
    cerr << "ERROR: Wrong number of edges (" << g->GetNumberOfEdges() << " != " << 299 << ")"
         << endl;
    errors++;
  }
  SVTK_CREATE(svtkBitArray, visited);
  visited->SetNumberOfTuples(g->GetNumberOfVertices());
  for (svtkIdType i = 0; i < g->GetNumberOfVertices(); i++)
  {
    visited->SetValue(i, 0);
  }
  SVTK_CREATE(svtkIdTypeArray, stack);
  stack->SetNumberOfTuples(g->GetNumberOfVertices());
  svtkIdType top = 0;
  stack->SetValue(top, 0);
  SVTK_CREATE(svtkAdjacentVertexIterator, adj);
  while (top >= 0)
  {
    svtkIdType u = stack->GetValue(top);
    top--;
    g->GetAdjacentVertices(u, adj);
    while (adj->HasNext())
    {
      svtkIdType v = adj->Next();
      if (!visited->GetValue(v))
      {
        visited->SetValue(v, 1);
        top++;
        stack->SetValue(top, v);
      }
    }
  }
  svtkIdType numVisited = 0;
  for (svtkIdType i = 0; i < g->GetNumberOfVertices(); i++)
  {
    numVisited += visited->GetValue(i);
  }
  if (numVisited != g->GetNumberOfVertices())
  {
    cerr << "ERROR: Starting with tree was not connected."
         << "Only " << numVisited << " of " << g->GetNumberOfVertices() << " were connected."
         << endl;
    errors++;
  }
  cerr << "...done." << endl;

  return errors;
}
