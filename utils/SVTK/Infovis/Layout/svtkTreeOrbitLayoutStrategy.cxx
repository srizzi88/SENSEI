/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeOrbitLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkTreeOrbitLayoutStrategy.h"

#include "svtkAbstractArray.h"
#include "svtkAdjacentVertexIterator.h"
#if SVTK_MODULE_ENABLE_SVTK_InfovisBoostGraphAlgorithms
#include "svtkBoostBreadthFirstSearchTree.h"
#endif
#include "svtkDataArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"

svtkStandardNewMacro(svtkTreeOrbitLayoutStrategy);

svtkTreeOrbitLayoutStrategy::svtkTreeOrbitLayoutStrategy()
{
  this->LogSpacingValue = 1.0;
  this->LeafSpacing = 1.0;
  this->ChildRadiusFactor = .5;
}

svtkTreeOrbitLayoutStrategy::~svtkTreeOrbitLayoutStrategy() = default;

// Helper method for recursively orbiting children
// around their parents
void svtkTreeOrbitLayoutStrategy::OrbitChildren(
  svtkTree* t, svtkPoints* p, svtkIdType parent, double radius)
{

  // Get the current position of the parent
  double pt[3];
  double xCenter, yCenter;
  p->GetPoint(parent, pt);
  xCenter = pt[0];
  yCenter = pt[1];

  // Check for leaf_count array
  svtkIntArray* leaf_count =
    svtkArrayDownCast<svtkIntArray>(t->GetVertexData()->GetArray("leaf_count"));
  if (!leaf_count)
  {
    svtkErrorMacro("svtkTreeOrbitLayoutStrategy has to have a leaf_count array");
    exit(1);
  }

  // Get the total number of children for this node
  double totalChildren = leaf_count->GetValue(parent);
  svtkIdType immediateChildren = t->GetNumberOfChildren(parent);

  // Now simply orbit the children around the
  // parent's centerpoint
  double currentAngle = 0;
  for (svtkIdType i = 0; i < immediateChildren; ++i)
  {
    svtkIdType childID = t->GetChild(parent, i);
    svtkIdType subChildren = leaf_count->GetValue(childID);

    // What angle do I get? If I have a lot of sub children
    // then I should get a greater angle 'pizza slice'
    double myAngle = subChildren / totalChildren;

    // So I want to be in the middle of my pizza slice
    double angle = currentAngle + myAngle / 2.0;

    // Compute coords
    double x = cos(2.0 * svtkMath::Pi() * angle);
    double y = sin(2.0 * svtkMath::Pi() * angle);

    // Am I a leaf
    double radiusFactor;
    if (subChildren == 1)
      radiusFactor = .1;
    else
      radiusFactor = log(static_cast<double>(immediateChildren)) / log(totalChildren);
    double xOrbit = x * radius * radiusFactor + xCenter;
    double yOrbit = y * radius * radiusFactor + yCenter;
    p->SetPoint(childID, xOrbit, yOrbit, 0);

    // Compute child radius
    double childRadius = radius * tan(myAngle) * 2.0 * this->ChildRadiusFactor;

    // Now recurse with a reduced radius
    this->OrbitChildren(t, p, childID, childRadius);

    // Accumulate angle
    currentAngle += myAngle;
  }
}

// Tree layout method
void svtkTreeOrbitLayoutStrategy::Layout()
{
  svtkTree* tree = svtkTree::SafeDownCast(this->Graph);
  if (tree == nullptr)
  {
#if SVTK_MODULE_ENABLE_SVTK_InfovisBoostGraphAlgorithms
    // Use the BFS search tree to perform the layout
    svtkBoostBreadthFirstSearchTree* bfs = svtkBoostBreadthFirstSearchTree::New();
    bfs->CreateGraphVertexIdArrayOn();
    bfs->SetInputData(this->Graph);
    bfs->Update();
    tree = svtkTree::New();
    tree->ShallowCopy(bfs->GetOutput());
    bfs->Delete();
#else
    svtkErrorMacro("Layout only works on svtkTree if SVTK::InfovisBoostGraphAlgorithms is available.");
#endif
  }

  if (tree->GetNumberOfVertices() == 0)
  {
    svtkErrorMacro("Tree Input has 0 vertices - Punting...");
    return;
  }

  // Create a new point set
  svtkIdType numVertices = tree->GetNumberOfVertices();
  svtkPoints* newPoints = svtkPoints::New();
  newPoints->SetNumberOfPoints(numVertices);

  // Setting the root to position 0,0 but this could
  // be whatever you want and should be controllable
  // through ivars in the future
  svtkIdType currentRoot = tree->GetRoot();
  newPoints->SetPoint(currentRoot, 0, 0, 0);

  // Now traverse the tree and have all children
  // orbit their parents recursively
  this->OrbitChildren(tree, newPoints, currentRoot, 1);

  // Copy coordinates back into the original graph
  if (svtkTree::SafeDownCast(this->Graph))
  {
    this->Graph->SetPoints(newPoints);
  }
#if SVTK_MODULE_ENABLE_SVTK_InfovisBoostGraphAlgorithms
  else
  {
    // Reorder the points based on the mapping back to graph vertex ids
    svtkPoints* reordered = svtkPoints::New();
    reordered->SetNumberOfPoints(newPoints->GetNumberOfPoints());
    for (svtkIdType i = 0; i < reordered->GetNumberOfPoints(); i++)
    {
      reordered->SetPoint(i, 0, 0, 0);
    }
    svtkIdTypeArray* graphVertexIdArr =
      svtkArrayDownCast<svtkIdTypeArray>(tree->GetVertexData()->GetAbstractArray("GraphVertexId"));
    for (svtkIdType i = 0; i < graphVertexIdArr->GetNumberOfTuples(); i++)
    {
      reordered->SetPoint(graphVertexIdArr->GetValue(i), newPoints->GetPoint(i));
    }
    this->Graph->SetPoints(reordered);
    tree->Delete();
    reordered->Delete();
  }
#endif

  // Clean up.
  newPoints->Delete();
}

void svtkTreeOrbitLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogSpacingValue: " << this->LogSpacingValue << endl;
  os << indent << "LeafSpacing: " << this->LeafSpacing << endl;
  os << indent << "ChildRadiusFactor: " << this->ChildRadiusFactor << endl;
}
