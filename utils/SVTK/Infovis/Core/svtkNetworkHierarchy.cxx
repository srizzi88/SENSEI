/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNetworkHierarchy.cxx

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

#include "svtkNetworkHierarchy.h"

#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"
#include "svtkVariant.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkNetworkHierarchy);

// This is just a macro wrapping for smart pointers
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkNetworkHierarchy::svtkNetworkHierarchy()
{
  this->IPArrayName = nullptr;
  this->SetIPArrayName("ip");
}

svtkNetworkHierarchy::~svtkNetworkHierarchy()
{
  this->SetIPArrayName(nullptr);
}

void svtkNetworkHierarchy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IPArrayName: " << (this->IPArrayName ? "" : "(null)") << endl;
}

int svtkNetworkHierarchy::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTree");
  return 1;
}

int svtkNetworkHierarchy::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkNetworkHierarchy::GetSubnets(unsigned int packedIP, int* subnets)
{
  unsigned int num = packedIP;
  subnets[3] = num % 256;
  num = num >> 8;
  subnets[2] = num % 256;
  num = num >> 8;
  subnets[1] = num % 256;
  num = num >> 8;
  subnets[0] = num;
}

unsigned int svtkNetworkHierarchy::ITON(const svtkStdString& ip)
{
  unsigned int subnets[4];
  sscanf(ip.c_str(), "%u.%u.%u.%u", &(subnets[0]), &(subnets[1]), &(subnets[2]), &(subnets[3]));
  int num = subnets[0];
  num = num << 8;
  num += subnets[1];
  num = num << 8;
  num += subnets[2];
  num = num << 8;
  num += subnets[3];
  return num;
}

int svtkNetworkHierarchy::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTable and outputTree handles
  svtkGraph* inputGraph = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* outputTree = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Get the field to filter on
  svtkAbstractArray* arr = inputGraph->GetVertexData()->GetAbstractArray(this->IPArrayName);
  svtkStringArray* ipArray = svtkArrayDownCast<svtkStringArray>(arr);
  if (ipArray == nullptr)
  {
    svtkErrorMacro(<< "An string based ip array must be specified");
    return 0;
  }

  // Build subnet map
  typedef std::vector<std::pair<unsigned int, svtkIdType> > subnet_map_type;
  subnet_map_type SubnetMap;
  for (svtkIdType i = 0; i < ipArray->GetNumberOfTuples(); ++i)
  {
    unsigned int packedID = this->ITON(ipArray->GetValue(i));
    SubnetMap.push_back(std::make_pair(packedID, i));
  }
  std::sort(SubnetMap.begin(), SubnetMap.end());

  // Create builder for the tree
  SVTK_CREATE(svtkMutableDirectedGraph, builder);

  // Make a bunch of blank vertices
  for (int i = 0; i < inputGraph->GetNumberOfVertices(); ++i)
  {
    builder->AddVertex();
  }

  // Get the input graph and copy the vertex data
  svtkDataSetAttributes* inputVertexData = inputGraph->GetVertexData();
  svtkDataSetAttributes* builderVertexData = builder->GetVertexData();
  builderVertexData->DeepCopy(inputVertexData);

  // Get pedigree ids.
  svtkAbstractArray* pedIDArr = builderVertexData->GetPedigreeIds();

  // Get domain. If there isn't one, make one.
  svtkStringArray* domainArr =
    svtkArrayDownCast<svtkStringArray>(builderVertexData->GetAbstractArray("domain"));
  if (pedIDArr && !domainArr)
  {
    domainArr = svtkStringArray::New();
    domainArr->SetName("domain");
    for (svtkIdType r = 0; r < inputGraph->GetNumberOfVertices(); ++r)
    {
      domainArr->InsertNextValue(pedIDArr->GetName());
    }
    builderVertexData->AddArray(domainArr);
  }

  // All new vertices will be placed in this domain.
  svtkStdString newVertexDomain = "subnet";

  // Make the builder's field data a table
  // so we can call InsertNextBlankRow.
  svtkSmartPointer<svtkTable> treeTable = svtkSmartPointer<svtkTable>::New();
  treeTable->SetRowData(builder->GetVertexData());

  // Get the pedigree ID and domain columns
  int pedIDColumn = -1;
  int domainColumn = -1;
  if (pedIDArr)
  {
    treeTable->GetRowData()->GetAbstractArray(pedIDArr->GetName(), pedIDColumn);
    treeTable->GetRowData()->GetAbstractArray("domain", domainColumn);
  }

  // Add root
  svtkIdType rootID = builder->AddVertex();
  treeTable->InsertNextBlankRow();

  // Don't label the root node...
  // treeTable->SetValueByName(rootID, this->IPArrayName, svtkVariant("Internet"));
  treeTable->SetValueByName(rootID, this->IPArrayName, svtkVariant(""));
  if (pedIDArr)
  {
    treeTable->SetValue(rootID, pedIDColumn, rootID);
    treeTable->SetValue(rootID, domainColumn, newVertexDomain);
  }

  // Iterate through the different subnets
  subnet_map_type::iterator I;
  int currentSubnet0 = -1;
  int currentSubnet1 = -1;
  int currentSubnet2 = -1;
  int subnets[4];
  svtkIdType currentParent0 = 0;
  svtkIdType currentParent1 = 0;
  svtkIdType currentParent2 = 0;
  svtkIdType treeIndex;
  svtkIdType leafIndex;
  for (I = SubnetMap.begin(); I != SubnetMap.end(); ++I)
  {
    unsigned int packedID = (*I).first;
    leafIndex = (*I).second;
    this->GetSubnets(packedID, subnets);

    // Is this a new subnet 0
    if (subnets[0] != currentSubnet0)
    {
      // Add child
      treeIndex = builder->AddChild(rootID);

      // Add vertex fields for the child
      treeTable->InsertNextBlankRow();

      // Set the label for the child
      std::ostringstream subnetStream;
      subnetStream << subnets[0];
      treeTable->SetValueByName(treeIndex, this->IPArrayName, svtkVariant(subnetStream.str()));

      // Set pedigree ID and domain for the child
      if (pedIDArr)
      {
        treeTable->SetValue(treeIndex, pedIDColumn, treeIndex);
        treeTable->SetValue(treeIndex, domainColumn, newVertexDomain);
      }

      // Store new parent/subnet info
      currentSubnet0 = subnets[0];
      currentParent0 = treeIndex;

      // Invalidate subnets
      currentSubnet1 = currentSubnet2 = -1;
    }

    // Is this a new subnet 1
    if (subnets[1] != currentSubnet1)
    {
      // Add child
      treeIndex = builder->AddChild(currentParent0);

      // Add vertex fields for the child
      treeTable->InsertNextBlankRow();

      // Set the label for the child
      std::ostringstream subnetStream;
      subnetStream << subnets[0] << "." << subnets[1];
      treeTable->SetValueByName(treeIndex, this->IPArrayName, svtkVariant(subnetStream.str()));

      // Set pedigree ID and domain for the child
      if (pedIDArr)
      {
        treeTable->SetValue(treeIndex, pedIDColumn, treeIndex);
        treeTable->SetValue(treeIndex, domainColumn, newVertexDomain);
      }

      // Store new parent/subnet info
      currentSubnet1 = subnets[1];
      currentParent1 = treeIndex;

      // Invalidate subnets
      currentSubnet2 = -1;
    }

    // Is this a new subnet 2
    if (subnets[2] != currentSubnet2)
    {
      // Add child
      treeIndex = builder->AddChild(currentParent1);

      // Add vertex fields for the child
      treeTable->InsertNextBlankRow();

      // Set the label for the child
      std::ostringstream subnetStream;
      subnetStream << subnets[0] << "." << subnets[1] << "." << subnets[2];
      treeTable->SetValueByName(treeIndex, this->IPArrayName, svtkVariant(subnetStream.str()));

      // Set pedigree ID and domain for the child
      if (pedIDArr)
      {
        treeTable->SetValue(treeIndex, pedIDColumn, treeIndex);
        treeTable->SetValue(treeIndex, domainColumn, newVertexDomain);
      }

      // Store new parent/subnet info
      currentSubnet2 = subnets[2];
      currentParent2 = treeIndex;
    }

    builder->AddEdge(currentParent2, leafIndex);
  }

  // Move the structure to the output
  if (!outputTree->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid tree structure!");
    return 0;
  }

  return 1;
}
