/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkIncrementalForceLayout.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkRandomGraphSource.h"

//----------------------------------------------------------------------------
int TestIncrementalForceLayout(int, char*[])
{
  svtkNew<svtkRandomGraphSource> source;
  source->SetNumberOfVertices(10);
  source->SetStartWithTree(false);
  source->SetNumberOfEdges(10);
  source->Update();

  svtkGraph* randomGraph = source->GetOutput();
  for (svtkIdType i = 0; i < randomGraph->GetNumberOfVertices(); ++i)
  {
    randomGraph->GetPoints()->SetPoint(i, svtkMath::Random(), svtkMath::Random(), svtkMath::Random());
  }

  svtkNew<svtkIncrementalForceLayout> layout;
  layout->SetGraph(randomGraph);
  layout->SetDistance(20.0f);
  for (svtkIdType i = 0; i < 1000; ++i)
  {
    layout->UpdatePositions();
  }
  for (svtkIdType e = 0; e < randomGraph->GetNumberOfEdges(); ++e)
  {
    if (randomGraph->GetSourceVertex(e) == randomGraph->GetTargetVertex(e))
    {
      continue;
    }
    double p1[3];
    randomGraph->GetPoint(randomGraph->GetSourceVertex(e), p1);
    double p2[3];
    randomGraph->GetPoint(randomGraph->GetTargetVertex(e), p2);
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dist = sqrt(dx * dx + dy * dy);
    std::cerr << "Edge distance: " << dist << std::endl;
    if (fabs(dist - 20.0) > 5.0)
    {
      return 1;
    }
  }

  return 0;
}
