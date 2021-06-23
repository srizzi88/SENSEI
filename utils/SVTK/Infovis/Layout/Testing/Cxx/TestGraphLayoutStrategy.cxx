/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraphLayoutStrategy.cxx

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
#include "svtkCircularLayoutStrategy.h"
#include "svtkEdgeListIterator.h"
#include "svtkFast2DLayoutStrategy.h"
#include "svtkForceDirectedLayoutStrategy.h"
#include "svtkGraphLayout.h"
#include "svtkMath.h"
#include "svtkPassThroughLayoutStrategy.h"
#include "svtkRandomGraphSource.h"
#include "svtkRandomLayoutStrategy.h"
#include "svtkSimple2DLayoutStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTreeLayoutStrategy.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestGraphLayoutStrategy(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int errors = 0;

  // Create input graph
  svtkIdType numVert = 100;
  svtkIdType numEdges = 150;
  SVTK_CREATE(svtkRandomGraphSource, source);
  source->SetNumberOfVertices(numVert);
  source->SetNumberOfEdges(numEdges);

  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputConnection(source->GetOutputPort());
  svtkGraph* output = nullptr;
  double pt[3] = { 0.0, 0.0, 0.0 };
  double pt2[3] = { 0.0, 0.0, 0.0 };
  double eps = 1.0e-6;
  double length = 0.0;
  double tol = 50.0;

  cerr << "Testing svtkCircularLayoutStrategy..." << endl;
  SVTK_CREATE(svtkCircularLayoutStrategy, circular);
  layout->SetLayoutStrategy(circular);
  layout->Update();
  output = layout->GetOutput();
  for (svtkIdType i = 0; i < numVert; i++)
  {
    output->GetPoint(i, pt);
    double dist = pt[0] * pt[0] + pt[1] * pt[1] - 1.0;
    dist = dist > 0 ? dist : -dist;
    if (dist > eps || pt[2] != 0.0)
    {
      cerr << "ERROR: Point " << i << " is not on the unit circle." << endl;
      errors++;
    }
  }
  cerr << "...done." << endl;

  cerr << "Testing svtkFast2DLayoutStrategy..." << endl;
  SVTK_CREATE(svtkFast2DLayoutStrategy, fast);
  fast->SetRestDistance(1.0f);
  length = fast->GetRestDistance();
  layout->SetLayoutStrategy(fast);
  layout->Update();
  output = layout->GetOutput();
  SVTK_CREATE(svtkEdgeListIterator, edges);
  output->GetEdges(edges);
  while (edges->HasNext())
  {
    svtkEdgeType e = edges->Next();
    svtkIdType u = e.Source;
    svtkIdType v = e.Target;
    output->GetPoint(u, pt);
    output->GetPoint(v, pt2);
    double dist = sqrt(svtkMath::Distance2BetweenPoints(pt, pt2));
    if (dist < length / tol || dist > length * tol)
    {
      cerr << "ERROR: Edge " << u << "," << v << " distance is " << dist
           << " but resting distance is " << length << endl;
      errors++;
    }
    if (pt[2] != 0.0)
    {
      cerr << "ERROR: Point " << u << " not on the xy plane" << endl;
      errors++;
    }
    if (pt2[2] != 0.0)
    {
      cerr << "ERROR: Point " << v << " not on the xy plane" << endl;
      errors++;
    }
  }
  cerr << "...done." << endl;

  cerr << "Testing svtkForceDirectedLayoutStrategy..." << endl;
  SVTK_CREATE(svtkForceDirectedLayoutStrategy, force);
  length = pow(1.0 / numVert, 1.0 / 3.0);
  layout->SetLayoutStrategy(force);
  layout->Update();
  output = layout->GetOutput();
  output->GetEdges(edges);
  while (edges->HasNext())
  {
    svtkEdgeType e = edges->Next();
    svtkIdType u = e.Source;
    svtkIdType v = e.Target;
    output->GetPoint(u, pt);
    output->GetPoint(v, pt2);
    double dist = sqrt(svtkMath::Distance2BetweenPoints(pt, pt2));
    if (dist < length / tol || dist > length * tol)
    {
      cerr << "ERROR: Edge " << u << "," << v << " distance is " << dist
           << " but resting distance is " << length << endl;
      errors++;
    }
  }
  cerr << "...done." << endl;

  cerr << "Testing svtkPassThroughLayoutStrategy..." << endl;
  SVTK_CREATE(svtkPassThroughLayoutStrategy, pass);
  layout->SetLayoutStrategy(pass);
  layout->Update();
  output = layout->GetOutput();
  for (svtkIdType i = 0; i < numVert; i++)
  {
    output->GetPoint(i, pt);
    if (pt[0] != 0.0 || pt[1] != 0.0 || pt[2] != 0.0)
    {
      cerr << "ERROR: Point " << i << " is not 0,0,0." << endl;
      errors++;
    }
  }
  cerr << "...done." << endl;

  cerr << "Testing svtkRandomLayoutStrategy..." << endl;
  SVTK_CREATE(svtkRandomLayoutStrategy, random);
  double bounds[6] = { 0, 0, 0, 0, 0, 0 };
  random->GetGraphBounds(bounds);
  layout->SetLayoutStrategy(random);
  layout->Update();
  output = layout->GetOutput();
  for (svtkIdType i = 0; i < numVert; i++)
  {
    output->GetPoint(i, pt);
    if (pt[0] < bounds[0] || pt[0] > bounds[1] || pt[1] < bounds[2] || pt[1] > bounds[3] ||
      pt[2] < bounds[4] || pt[2] > bounds[5])
    {
      cerr << "ERROR: Point " << i << " is not within the bounds." << endl;
      errors++;
    }
  }
  cerr << "...done." << endl;

  cerr << "Testing svtkSimple2DLayoutStrategy..." << endl;
  SVTK_CREATE(svtkSimple2DLayoutStrategy, simple);
  simple->SetRestDistance(1.0f);
  length = simple->GetRestDistance();
  layout->SetLayoutStrategy(simple);
  layout->Update();
  output = layout->GetOutput();
  output->GetEdges(edges);
  while (edges->HasNext())
  {
    svtkEdgeType e = edges->Next();
    svtkIdType u = e.Source;
    svtkIdType v = e.Target;
    output->GetPoint(u, pt);
    output->GetPoint(v, pt2);
    double dist = sqrt(svtkMath::Distance2BetweenPoints(pt, pt2));
    if (dist < length / tol || dist > length * tol)
    {
      cerr << "ERROR: Edge " << u << "," << v << " distance is " << dist
           << " but resting distance is " << length << endl;
      errors++;
    }
    if (pt[2] != 0.0)
    {
      cerr << "ERROR: Point " << u << " not on the xy plane" << endl;
      errors++;
    }
    if (pt2[2] != 0.0)
    {
      cerr << "ERROR: Point " << v << " not on the xy plane" << endl;
      errors++;
    }
  }
  cerr << "...done." << endl;

  return errors;
}
