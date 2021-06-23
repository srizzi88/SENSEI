/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeDifferenceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTreeDifferenceFilter.h"

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkTreeDifferenceFilter);

//---------------------------------------------------------------------------
svtkTreeDifferenceFilter::svtkTreeDifferenceFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->IdArrayName = nullptr;
  this->ComparisonArrayName = nullptr;
  this->OutputArrayName = nullptr;
  this->ComparisonArrayIsVertexData = false;
}

//---------------------------------------------------------------------------
svtkTreeDifferenceFilter::~svtkTreeDifferenceFilter()
{
  // release memory
  this->SetIdArrayName(nullptr);
  this->SetComparisonArrayName(nullptr);
  this->SetOutputArrayName(nullptr);
}

//---------------------------------------------------------------------------
int svtkTreeDifferenceFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//---------------------------------------------------------------------------
int svtkTreeDifferenceFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* tree1_info = inputVector[0]->GetInformationObject(0);
  svtkTree* tree1 = svtkTree::SafeDownCast(tree1_info->Get(svtkDataObject::DATA_OBJECT()));

  // Copy the structure into the output.
  svtkTree* outputTree = svtkTree::GetData(outputVector);

  svtkInformation* tree2_info = inputVector[1]->GetInformationObject(0);
  if (!tree2_info)
  {
    // If no second tree provided, we're done
    outputTree->CheckedShallowCopy(tree1);
    return 0;
  }

  svtkTree* tree2 = svtkTree::SafeDownCast(tree2_info->Get(svtkDataObject::DATA_OBJECT()));

  if (this->IdArrayName != nullptr)
  {
    if (!this->GenerateMapping(tree1, tree2))
    {
      return 0;
    }
  }
  else
  {
    this->VertexMap.clear();
    for (svtkIdType vertex = 0; vertex < tree1->GetNumberOfVertices(); ++vertex)
    {
      this->VertexMap[vertex] = vertex;
    }

    this->EdgeMap.clear();
    for (svtkIdType edge = 0; edge < tree1->GetNumberOfEdges(); ++edge)
    {
      this->EdgeMap[edge] = edge;
    }
  }

  svtkSmartPointer<svtkDoubleArray> resultArray = this->ComputeDifference(tree1, tree2);

  if (!outputTree->CheckedShallowCopy(tree1))
  {
    svtkErrorMacro(<< "Invalid tree structure.");
    return 0;
  }

  if (this->ComparisonArrayIsVertexData)
  {
    outputTree->GetVertexData()->AddArray(resultArray);
  }
  else
  {
    outputTree->GetEdgeData()->AddArray(resultArray);
  }

  return 1;
}

//---------------------------------------------------------------------------
bool svtkTreeDifferenceFilter::GenerateMapping(svtkTree* tree1, svtkTree* tree2)
{
  this->VertexMap.clear();
  this->VertexMap.assign(tree1->GetNumberOfVertices(), -1);

  this->EdgeMap.clear();
  this->EdgeMap.assign(tree1->GetNumberOfEdges(), -1);

  svtkStringArray* nodeNames1 =
    svtkArrayDownCast<svtkStringArray>(tree1->GetVertexData()->GetAbstractArray(this->IdArrayName));
  if (nodeNames1 == nullptr)
  {
    svtkErrorMacro(
      "tree #1's VertexData does not have a svtkStringArray named " << this->IdArrayName);
    return false;
  }

  svtkStringArray* nodeNames2 =
    svtkArrayDownCast<svtkStringArray>(tree2->GetVertexData()->GetAbstractArray(this->IdArrayName));
  if (nodeNames2 == nullptr)
  {
    svtkErrorMacro(
      "tree #2's VertexData does not have a svtkStringArray named " << this->IdArrayName);
    return false;
  }

  svtkIdType root1 = tree1->GetRoot();
  svtkIdType root2 = tree2->GetRoot();
  this->VertexMap[root1] = root2;

  svtkIdType edgeId1 = -1;
  svtkIdType edgeId2 = -1;

  // iterate over the vertex names for tree #1, finding the corresponding
  // vertex in tree #2.
  for (svtkIdType vertexItr = 0; vertexItr < nodeNames1->GetNumberOfTuples(); ++vertexItr)
  {
    svtkIdType vertexId1 = vertexItr;
    std::string nodeName = nodeNames1->GetValue(vertexId1);
    if (nodeName.compare("") == 0)
    {
      continue;
    }

    // record this correspondence in the maps
    svtkIdType vertexId2 = nodeNames2->LookupValue(nodeName);
    if (vertexId2 == -1)
    {
      svtkWarningMacro("tree #2 does not contain a vertex named " << nodeName);
      continue;
    }
    this->VertexMap[vertexId1] = vertexId2;

    if (vertexId1 == root1 || vertexId2 == root2)
    {
      continue;
    }

    edgeId1 = tree1->GetEdgeId(tree1->GetParent(vertexId1), vertexId1);
    edgeId2 = tree2->GetEdgeId(tree2->GetParent(vertexId2), vertexId2);
    this->EdgeMap[edgeId1] = edgeId2;

    // ascend the tree until we reach the root, mapping parent vertices to
    // each other along the way.
    while (tree1->GetParent(vertexId1) != root1 && tree2->GetParent(vertexId2) != root2)
    {
      vertexId1 = tree1->GetParent(vertexId1);
      vertexId2 = tree2->GetParent(vertexId2);
      if (this->VertexMap[vertexId1] == -1)
      {
        this->VertexMap[vertexId1] = vertexId2;
        edgeId1 = tree1->GetEdgeId(tree1->GetParent(vertexId1), vertexId1);
        edgeId2 = tree2->GetEdgeId(tree2->GetParent(vertexId2), vertexId2);
        this->EdgeMap[edgeId1] = edgeId2;
      }
    }
  }

  return true;
}

//---------------------------------------------------------------------------
svtkSmartPointer<svtkDoubleArray> svtkTreeDifferenceFilter::ComputeDifference(
  svtkTree* tree1, svtkTree* tree2)
{
  if (this->ComparisonArrayName == nullptr)
  {
    svtkErrorMacro("ComparisonArrayName has not been set.");
    return nullptr;
  }

  svtkDataSetAttributes *treeData1, *treeData2;
  const char* dataName;
  if (this->ComparisonArrayIsVertexData)
  {
    treeData1 = tree1->GetVertexData();
    treeData2 = tree2->GetVertexData();
    dataName = "VertexData";
  }
  else
  {
    treeData1 = tree1->GetEdgeData();
    treeData2 = tree2->GetEdgeData();
    dataName = "EdgeData";
  }

  svtkDataArray* arrayToCompare1 = treeData1->GetArray(this->ComparisonArrayName);
  if (arrayToCompare1 == nullptr)
  {
    svtkErrorMacro("tree #1's " << dataName << " does not have a svtkDoubleArray named "
                               << this->ComparisonArrayName);
    return nullptr;
  }

  svtkDataArray* arrayToCompare2 = treeData2->GetArray(this->ComparisonArrayName);
  if (arrayToCompare2 == nullptr)
  {
    svtkErrorMacro("tree #2's " << dataName << " does not have a svtkDoubleArray named "
                               << this->ComparisonArrayName);
    return nullptr;
  }

  svtkSmartPointer<svtkDoubleArray> resultArray = svtkSmartPointer<svtkDoubleArray>::New();
  resultArray->SetNumberOfValues(arrayToCompare1->GetNumberOfTuples());
  resultArray->FillComponent(0, svtkMath::Nan());

  if (this->OutputArrayName == nullptr)
  {
    resultArray->SetName("difference");
  }
  else
  {
    resultArray->SetName(this->OutputArrayName);
  }

  svtkIdType treeId2;
  for (svtkIdType treeId1 = 0; treeId1 < arrayToCompare1->GetNumberOfTuples(); ++treeId1)
  {
    if (this->ComparisonArrayIsVertexData)
    {
      treeId2 = this->VertexMap[treeId1];
    }
    else
    {
      treeId2 = this->EdgeMap[treeId1];
    }
    double result = arrayToCompare1->GetTuple1(treeId1) - arrayToCompare2->GetTuple1(treeId2);
    resultArray->SetValue(treeId1, result);
  }

  return resultArray;
}

//---------------------------------------------------------------------------
void svtkTreeDifferenceFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IdArrayName)
  {
    os << indent << "IdArrayName: " << this->IdArrayName << std::endl;
  }
  else
  {
    os << indent << "IdArrayName: "
       << "(None)" << std::endl;
  }
  if (this->ComparisonArrayName)
  {
    os << indent << "ComparisonArrayName: " << this->ComparisonArrayName << std::endl;
  }
  else
  {
    os << indent << "ComparisonArrayName: "
       << "(None)" << std::endl;
  }
  if (this->OutputArrayName)
  {
    os << indent << "OutputArrayName: " << this->OutputArrayName << std::endl;
  }
  else
  {
    os << indent << "OutputArrayName: "
       << "(None)" << std::endl;
  }
  os << indent << "ComparisonArrayIsVertexData: " << this->ComparisonArrayIsVertexData << std::endl;
}
