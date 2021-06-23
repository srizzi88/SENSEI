/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphHierarchicalBundleEdges.cxx

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
#include "svtkGraphHierarchicalBundleEdges.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkEdgeListIterator.h"
#include "svtkFloatArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkTree.h"
#include "svtkVariantArray.h"

#include <map>

svtkStandardNewMacro(svtkGraphHierarchicalBundleEdges);

svtkGraphHierarchicalBundleEdges::svtkGraphHierarchicalBundleEdges()
{
  this->BundlingStrength = 0.8;
  this->SetNumberOfInputPorts(2);
  this->DirectMapping = false;
}

int svtkGraphHierarchicalBundleEdges::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
    return 1;
  }
  return 0;
}

int svtkGraphHierarchicalBundleEdges::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* graphInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* treeInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* graph = svtkGraph::SafeDownCast(graphInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* tree = svtkTree::SafeDownCast(treeInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If graph or tree is empty, we're done.
  if (graph->GetNumberOfVertices() == 0 || tree->GetNumberOfVertices() == 0)
  {
    return 1;
  }

  // Create a map from graph indices to tree indices
  // If we are using DirectMapping this is trivial
  // we just create an identity map
  std::map<svtkIdType, svtkIdType> graphIndexToTreeIndex;
  if (this->DirectMapping)
  {
    if (graph->GetNumberOfVertices() > tree->GetNumberOfVertices())
    {
      svtkErrorMacro("Cannot have more graph vertices than tree vertices using direct mapping.");
      return 0;
    }
    // Create identity map.
    for (svtkIdType gv = 0; gv < graph->GetNumberOfVertices(); gv++)
    {
      graphIndexToTreeIndex[gv] = gv;
    }
  }

  // Okay if we do not have direct mapping then we need
  // to do some templated madness to go from an arbitrary
  // type to a nice svtkIdType to svtkIdType mapping
  if (!this->DirectMapping)
  {
    // Check for valid pedigree id arrays.
    svtkAbstractArray* graphIdArray = graph->GetVertexData()->GetPedigreeIds();
    if (!graphIdArray)
    {
      svtkErrorMacro("Graph pedigree id array not found.");
      return 0;
    }
    // Check for valid domain array, if any.
    svtkAbstractArray* graphDomainArray = graph->GetVertexData()->GetAbstractArray("domain");

    svtkAbstractArray* treeIdArray = tree->GetVertexData()->GetPedigreeIds();
    if (!treeIdArray)
    {
      svtkErrorMacro("Tree pedigree id array not found.");
      return 0;
    }
    // Check for valid domain array, if any.
    svtkAbstractArray* treeDomainArray = tree->GetVertexData()->GetAbstractArray("domain");

    std::map<svtkVariant, svtkIdType, svtkVariantLessThan> graphIdMap;

    // Create a map from graph id to graph index
    for (int i = 0; i < graph->GetNumberOfVertices(); ++i)
    {
      graphIdMap[graphIdArray->GetVariantValue(i)] = i;
    }

    // Now create the map from graph index to tree index
    for (int i = 0; i < tree->GetNumberOfVertices(); ++i)
    {
      svtkVariant id = treeIdArray->GetVariantValue(i);
      if (graphIdMap.count(id))
      {
        // Make sure that the domain for this id in the graph matches
        // the one in the tree before adding to the map. This guards
        // against drawing edges to group nodes in the tree.
        if (treeDomainArray)
        {
          svtkVariant treeDomain = treeDomainArray->GetVariantValue(i);
          svtkVariant graphDomain;
          if (graphDomainArray)
          {
            graphDomain = graphDomainArray->GetVariantValue(graphIdMap[id]);
          }
          else
          {
            graphDomain = graphIdArray->GetName();
          }
          if (graphDomain != treeDomain)
          {
            continue;
          }
        }

        graphIndexToTreeIndex[graphIdMap[id]] = i;
      }
    }
  }

  output->ShallowCopy(graph);
  output->DeepCopyEdgePoints(graph);
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  double pt[3];
  for (svtkIdType v = 0; v < graph->GetNumberOfVertices(); ++v)
  {
    svtkIdType treeVertex = 0;
    if (graphIndexToTreeIndex.count(v) > 0)
    {
      treeVertex = graphIndexToTreeIndex[v];
      tree->GetPoint(treeVertex, pt);
    }
    else
    {
      pt[0] = 0.0;
      pt[1] = 0.0;
      pt[2] = 0.0;
    }
    points->InsertNextPoint(pt);
  }
  output->SetPoints(points);

  svtkSmartPointer<svtkIdList> sourceList = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkIdList> targetList = svtkSmartPointer<svtkIdList>::New();
  svtkSmartPointer<svtkEdgeListIterator> edges = svtkSmartPointer<svtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
  {
    svtkEdgeType e = edges->Next();
    unsigned int graphSourceIndex = e.Source;
    unsigned int graphTargetIndex = e.Target;

    // Do not render loops
    if (graphSourceIndex == graphTargetIndex)
    {
      continue;
    }

    svtkIdType source = 0;
    svtkIdType target = 0;
    if (graphIndexToTreeIndex.count(graphSourceIndex) > 0 &&
      graphIndexToTreeIndex.count(graphTargetIndex) > 0)
    {
      source = graphIndexToTreeIndex[graphSourceIndex];
      target = graphIndexToTreeIndex[graphTargetIndex];
    }
    else
    {
      // The endpoints of this edge are not found in the tree.
      continue;
    }

    // Find path from source to target
    sourceList->Reset();
    svtkIdType curSource = source;
    while (curSource != tree->GetRoot())
    {
      curSource = tree->GetParent(curSource);
      sourceList->InsertNextId(curSource);
    }
    targetList->Reset();
    svtkIdType curTarget = target;
    while (sourceList->IsId(curTarget) == -1 && curTarget != source)
    {
      curTarget = tree->GetParent(curTarget);
      targetList->InsertNextId(curTarget);
    }

    svtkIdType cellPoints;
    if (curTarget == source)
    {
      cellPoints = targetList->GetNumberOfIds();
    }
    else
    {
      cellPoints = sourceList->IsId(curTarget) + targetList->GetNumberOfIds();
    }

    // We may eliminate a common ancestor if:
    // 1. The source is not an ancestor of the target
    // 2. The target is not an ancestor of the source
    // 3. The number of points along the path is at least 4
    bool eliminateCommonAncestor = false;
    if (sourceList->IsId(target) == -1 && targetList->IsId(source) == -1 && cellPoints >= 4)
    {
      cellPoints--;
      eliminateCommonAncestor = true;
    }

    double cellPointsD = static_cast<double>(cellPoints);
    double sourcePt[3];
    tree->GetPoint(source, sourcePt);
    double targetPt[3];
    tree->GetPoint(target, targetPt);

    double interpPt[3];
    int curPoint = 1;

    // Insert points into the polyline going up the tree to
    // the common ancestor.
    output->ClearEdgePoints(e.Id);
    for (svtkIdType s = 0; s < sourceList->IsId(curTarget); s++)
    {
      tree->GetPoint(sourceList->GetId(s), pt);
      for (int c = 0; c < 3; c++)
      {
        interpPt[c] = (1.0 - curPoint / (cellPointsD + 1)) * sourcePt[c] +
          (curPoint / (cellPointsD + 1)) * targetPt[c];
        interpPt[c] = (1.0 - this->BundlingStrength) * interpPt[c] + this->BundlingStrength * pt[c];
      }
      output->AddEdgePoint(e.Id, interpPt);
      ++curPoint;
    }

    // Insert points into the polyline going down the tree from
    // the common ancestor to the target vertex, possibly excluding
    // the common ancestor if it is a long path.
    svtkIdType maxTargetId = targetList->GetNumberOfIds() - 1;
    if (eliminateCommonAncestor)
    {
      maxTargetId = targetList->GetNumberOfIds() - 2;
    }
    for (svtkIdType t = maxTargetId; t >= 0; t--)
    {
      tree->GetPoint(targetList->GetId(t), pt);
      for (int c = 0; c < 3; c++)
      {
        interpPt[c] = (1.0 - curPoint / (cellPointsD + 1)) * sourcePt[c] +
          (curPoint / (cellPointsD + 1)) * targetPt[c];
        interpPt[c] = (1.0 - this->BundlingStrength) * interpPt[c] + this->BundlingStrength * pt[c];
      }
      output->AddEdgePoint(e.Id, interpPt);
      ++curPoint;
    }
  }

  return 1;
}

void svtkGraphHierarchicalBundleEdges::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BundlingStrength: " << this->BundlingStrength << endl;
  os << indent << "DirectMapping: " << this->DirectMapping << endl;
}
