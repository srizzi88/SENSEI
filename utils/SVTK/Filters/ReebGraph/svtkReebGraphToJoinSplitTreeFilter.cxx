/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReebGraphToJoinSplitTreeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkReebGraphToJoinSplitTreeFilter.h"

#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkReebGraph.h"
#include "svtkVariantArray.h"

#include <algorithm>
#include <boost/pending/disjoint_sets.hpp>

namespace
{
//----------------------------------------------------------------------------
inline static bool svtkReebGraphVertexSoS(
  const std::pair<int, double>& v0, const std::pair<int, double>& v1)
{
  return ((v0.second < v1.second) || ((v0.second == v1.second) && (v0.first < v1.first)));
}
}

svtkStandardNewMacro(svtkReebGraphToJoinSplitTreeFilter);

//----------------------------------------------------------------------------
svtkReebGraphToJoinSplitTreeFilter::svtkReebGraphToJoinSplitTreeFilter()
{
  this->SetNumberOfInputPorts(2);
  this->IsSplitTree = false;
  this->FieldId = 0;
}

//----------------------------------------------------------------------------
svtkReebGraphToJoinSplitTreeFilter::~svtkReebGraphToJoinSplitTreeFilter() {}

//----------------------------------------------------------------------------
int svtkReebGraphToJoinSplitTreeFilter::FillInputPortInformation(
  int portNumber, svtkInformation* info)
{

  switch (portNumber)
  {
    case 0:
      info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
      break;
    case 1:
      info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkReebGraph");
      break;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkReebGraphToJoinSplitTreeFilter::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDirectedGraph::DATA_TYPE_NAME(), "svtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void svtkReebGraphToJoinSplitTreeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Is Split Tree: " << this->IsSplitTree << "\n";
  os << indent << "Field Id: " << this->FieldId << "\n";
}

//----------------------------------------------------------------------------
svtkReebGraph* svtkReebGraphToJoinSplitTreeFilter::GetOutput()
{
  return svtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int svtkReebGraphToJoinSplitTreeFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkInformation *inInfoMesh = inputVector[0]->GetInformationObject(0),
                 *inInfoGraph = inputVector[1]->GetInformationObject(0);

  if ((!inInfoMesh) || (!inInfoGraph))
    return 0;

  svtkPointSet* inputMesh = svtkPointSet::SafeDownCast(inInfoMesh->Get(svtkPointSet::DATA_OBJECT()));

  svtkReebGraph* inputGraph =
    svtkReebGraph::SafeDownCast(inInfoGraph->Get(svtkReebGraph::DATA_OBJECT()));

  if ((inputMesh) && (inputGraph))
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    svtkReebGraph* output = svtkReebGraph::SafeDownCast(outInfo->Get(svtkReebGraph::DATA_OBJECT()));

    if (output)
    {
      output->DeepCopy(inputGraph);

      // Retrieve the information regarding the critical nodes
      svtkDataArray* vertexInfo =
        svtkArrayDownCast<svtkDataArray>(inputGraph->GetVertexData()->GetAbstractArray("Vertex Ids"));
      if (!vertexInfo)
        // invalid Reeb graph (no information associated to the vertices)
        return 0;

      svtkVariantArray* edgeInfo = svtkArrayDownCast<svtkVariantArray>(
        inputGraph->GetEdgeData()->GetAbstractArray("Vertex Ids"));
      if (!edgeInfo)
        // invalid Reeb graph (no information associated to the edges)
        return 0;

      svtkDataArray* scalarField = inputMesh->GetPointData()->GetArray(FieldId);
      if (!scalarField)
        // invalid input mesh (no scalar field associated to it)
        return 0;

      // first, uncompress the input Reeb graph.
      svtkMutableDirectedGraph* unCompressedGraph = svtkMutableDirectedGraph::New();
      std::vector<std::pair<int, double> > vertexList;
      for (int i = 0; i < vertexInfo->GetNumberOfTuples(); i++)
      {
        int vertexId = (int)*(vertexInfo->GetTuple(i));
        double scalarValue = scalarField->GetComponent(vertexId, 0);
        vertexList.push_back(std::pair<int, double>(vertexId, scalarValue));
      }

      svtkEdgeListIterator* eIt = svtkEdgeListIterator::New();
      inputGraph->GetEdges(eIt);
      do
      {
        svtkEdgeType e = eIt->Next();
        svtkAbstractArray* deg2NodeList = edgeInfo->GetPointer(e.Id)->ToArray();
        for (int i = 0; i < deg2NodeList->GetNumberOfTuples(); i++)
        {
          int vertexId = deg2NodeList->GetVariantValue(i).ToInt();
          double scalarValue = scalarField->GetComponent(vertexId, 0);
          vertexList.push_back(std::pair<int, double>(vertexId, scalarValue));
        }
      } while (eIt->HasNext());
      eIt->Delete();

      std::vector<int> vertexToNodeMap(vertexList.size());
      // create the nodes
      svtkVariantArray* vertexProperties = svtkVariantArray::New();
      vertexProperties->SetNumberOfValues(1);

      svtkIdTypeArray* vertexIds = svtkIdTypeArray::New();
      vertexIds->SetName("Vertex Ids");
      unCompressedGraph->GetVertexData()->AddArray(vertexIds);
      for (unsigned int i = 0; i < vertexList.size(); i++)
      {
        vertexProperties->SetValue(0, vertexList[i].first);
        unCompressedGraph->AddVertex(vertexProperties);
        vertexToNodeMap[vertexList[i].first] = i;
      }
      vertexIds->Delete();
      vertexProperties->Delete();

      eIt = svtkEdgeListIterator::New();
      inputGraph->GetEdges(eIt);
      do
      {
        svtkEdgeType e = eIt->Next();
        int sourceVertexId = (int)*(vertexInfo->GetTuple(e.Source)),
            targetVertexId = (int)*(vertexInfo->GetTuple(e.Target));
        svtkAbstractArray* deg2NodeList = edgeInfo->GetPointer(e.Id)->ToArray();

        if (!deg2NodeList->GetNumberOfTuples())
        {
          // empty arc
          unCompressedGraph->AddEdge(
            vertexToNodeMap[sourceVertexId], vertexToNodeMap[targetVertexId]);
        }
        else
        {
          unCompressedGraph->AddEdge(vertexToNodeMap[sourceVertexId],
            vertexToNodeMap[deg2NodeList->GetVariantValue(0).ToInt()]);

          for (int i = 1; i < deg2NodeList->GetNumberOfTuples(); i++)
          {
            unCompressedGraph->AddEdge(
              vertexToNodeMap[deg2NodeList->GetVariantValue(i - 1).ToInt()],
              vertexToNodeMap[deg2NodeList->GetVariantValue(i).ToInt()]);
          }

          unCompressedGraph->AddEdge(
            vertexToNodeMap[deg2NodeList->GetVariantValue(deg2NodeList->GetNumberOfTuples() - 1)
                              .ToInt()],
            vertexToNodeMap[targetVertexId]);
        }
      } while (eIt->HasNext());
      eIt->Delete();
      // input graph uncompressed.

      // now the actual join/split tree algorithm.

      // sort the vertices, by increasing order for join trees and
      // decreasing order for split trees.
      std::sort(vertexList.begin(), vertexList.end(), svtkReebGraphVertexSoS);

      if (IsSplitTree)
      {
        // reverse the list of vertices
        std::vector<std::pair<int, double> > tmpVector(vertexList);
        for (int i = static_cast<int>(tmpVector.size()) - 1; i >= 0; i--)
        {
          vertexList[vertexList.size() - i - 1] = tmpVector[i];
        }
      }

      // then, prepare the necessary adjacency information
      std::vector<std::vector<int> > halfStars(vertexList.size());

      vertexInfo = svtkArrayDownCast<svtkDataArray>(
        unCompressedGraph->GetVertexData()->GetAbstractArray("Vertex Ids"));
      eIt = svtkEdgeListIterator::New();
      unCompressedGraph->GetEdges(eIt);

      do
      {
        svtkEdgeType e = eIt->Next();
        int sourceId = (svtkIdType) * (vertexInfo->GetTuple(e.Source)),
            targetId = (svtkIdType) * (vertexInfo->GetTuple(e.Target));

        if (!IsSplitTree)
          halfStars[targetId].push_back(sourceId);
        else
          halfStars[sourceId].push_back(targetId);

      } while (eIt->HasNext());
      eIt->Delete();
      // at this point, we don't need the unCompressedGraph anymore now that we
      // have the halfStars correctly built.
      unCompressedGraph->Delete();

      // prepare the intermediate data-structure
      std::vector<std::pair<std::pair<int, int>, std::vector<int> > > edgeList(vertexList.size());
      for (unsigned int i = 0; i < edgeList.size(); i++)
      {
        edgeList[i].first.first = -1;
        edgeList[i].first.second = -1;
      }

      // prepare the unionFind
      std::vector<int> rank(vertexList.size());
      std::vector<int> parent(vertexList.size());
      boost::disjoint_sets<int*, int*> unionFind(&rank[0], &parent[0]);

      // enables a compressed usage of the UF
      std::vector<int> vertexToUFQueryMap(vertexList.size());

      int representative = 0;

      // we don't parse the last vertex, for sure it's gonna be the
      // global "max".
      for (unsigned int i = 0; i < vertexList.size() - 1; i++)
      {

        if (halfStars[vertexList[i].first].empty())
        {
          // this is leaf (either a min or a max)
          unionFind.make_set(vertexList[i].first);
          vertexToUFQueryMap[vertexList[i].first] = vertexList[i].first;
          representative = unionFind.find_set(vertexList[i].first);
          edgeList[representative].first.first = vertexList[i].first;
        }

        else
        {
          std::vector<int> representatives;
          // this is not a leaf node.
          // let's collect the unionFind representatives

          for (unsigned int j = 0; j < halfStars[vertexList[i].first].size(); j++)
          {
            representative =
              unionFind.find_set(vertexToUFQueryMap[halfStars[vertexList[i].first][j]]);

            // add it to the representative list
            bool isAlreadyStored = false;
            for (unsigned int k = 0; ((k < representatives.size()) && (!isAlreadyStored)); k++)
            {
              // 95% of time this will loop only once (depending on the field
              // complexity).
              //
              // if there is a non-degenerate (index 3) merge. it will loop
              // twice.
              //
              // High-index degenerate merge have a very very low probability of
              // appearance which is roughly inversely proportionnal to way more
              // than its index.
              if (representatives[k] == representative)
              {
                isAlreadyStored = true;
              }
            }
            if (!isAlreadyStored)
              representatives.push_back(representative);
          }

          if (representatives.size() == 1)
          {
            // add a deg2 node
            edgeList[representative].second.push_back(vertexList[i].first);
            // propagate the vertexId to be used to query the UF.
            vertexToUFQueryMap[vertexList[i].first] =
              vertexToUFQueryMap[halfStars[vertexList[i].first]
                                          [halfStars[vertexList[i].first].size() - 1]];
          }
          else
          {
            // close the incoming edges
            for (unsigned int j = 0; j < representatives.size(); j++)
            {
              edgeList[representatives[j]].first.second = vertexList[i].first;
            }
            // now open a new edge
            unionFind.make_set(vertexList[i].first);
            representative = unionFind.find_set(vertexList[i].first);
            for (unsigned int j = 0; j < representatives.size(); j++)
              unionFind.link(representatives[j], representative);

            vertexToUFQueryMap[vertexList[i].first] = vertexList[i].first;
            edgeList[representative].first.first = vertexList[i].first;
          }
        }
      }

      // put the global "max"
      representative = unionFind.find_set(
        vertexToUFQueryMap[halfStars[vertexList[vertexList.size() - 1].first][0]]);

      edgeList[representative].first.second = vertexList[vertexList.size() - 1].first;
      // join/split tree completed.

      // now format the output
      svtkMutableDirectedGraph* outputGraph = svtkMutableDirectedGraph::New();

      std::vector<int> criticalVertices;
      std::vector<bool> processedVertices(vertexList.size());
      for (unsigned int i = 0; i < processedVertices.size(); i++)
        processedVertices[i] = false;

      // first, create the list of nodes.
      for (unsigned int i = 0; i < edgeList.size(); i++)
      {
        if (edgeList[i].first.first != -1)
        {
          // valid edge
          if (!processedVertices[edgeList[i].first.first])
          {
            criticalVertices.push_back(edgeList[i].first.first);
            processedVertices[edgeList[i].first.first] = true;
          }
          if (!processedVertices[edgeList[i].first.second])
          {
            criticalVertices.push_back(edgeList[i].first.second);
            processedVertices[edgeList[i].first.second] = true;
          }
        }
      }

      vertexToNodeMap.resize(criticalVertices.size());

      vertexProperties = svtkVariantArray::New();
      vertexProperties->SetNumberOfValues(1);
      vertexIds = svtkIdTypeArray::New();
      vertexIds->SetName("Vertex Ids");
      outputGraph->GetVertexData()->AddArray(vertexIds);
      for (unsigned int i = 0; i < criticalVertices.size(); i++)
      {
        vertexProperties->SetValue(0, criticalVertices[i]);
        vertexToNodeMap[criticalVertices[i]] = i;
        outputGraph->AddVertex(vertexProperties);
      }
      vertexIds->Delete();
      vertexProperties->Delete();

      svtkVariantArray* deg2NodeIds = svtkVariantArray::New();
      deg2NodeIds->SetName("Vertex Ids");
      outputGraph->GetEdgeData()->AddArray(deg2NodeIds);

      for (unsigned int i = 0; i < edgeList.size(); i++)
      {
        if (edgeList[i].first.first != -1)
        {
          // valid edge
          int sourceNode = vertexToNodeMap[edgeList[i].first.first],
              targetNode = vertexToNodeMap[edgeList[i].first.second];
          svtkVariantArray* edgeProperties = svtkVariantArray::New();
          svtkIdTypeArray* vertexIdList = svtkIdTypeArray::New();
          vertexIdList->SetNumberOfValues(edgeList[i].second.size());
          for (unsigned int j = 0; j < edgeList[i].second.size(); j++)
            vertexIdList->SetValue(j, edgeList[i].second[j]);
          edgeProperties->SetNumberOfValues(1);
          edgeProperties->SetValue(0, vertexIdList);
          outputGraph->AddEdge(sourceNode, targetNode, edgeProperties);
          vertexIdList->Delete();
          edgeProperties->Delete();
        }
      }
      deg2NodeIds->Delete();

      output->Set(outputGraph);

      outputGraph->Delete();

      return 1;
    }
  }
  return 0;
}
