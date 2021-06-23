/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGroupLeafVertices.cxx

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

#include "svtkGroupLeafVertices.h"

#include "svtkIdList.h"
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
#include "svtkUnicodeStringArray.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include <map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkGroupLeafVertices);

// Forward function reference (definition at bottom :)
static int splitString(const svtkStdString& input, std::vector<svtkStdString>& results);

//---------------------------------------------------------------------------
class svtkGroupLeafVerticesCompare
{
public:
  bool operator()(
    const std::pair<svtkIdType, svtkVariant>& a, const std::pair<svtkIdType, svtkVariant>& b) const
  {
    if (a.first != b.first)
    {
      return a.first < b.first;
    }
    return svtkVariantLessThan()(a.second, b.second);
  }
};

//---------------------------------------------------------------------------
template <typename T>
svtkVariant svtkGroupLeafVerticesGetValue(T* arr, svtkIdType index)
{
  return svtkVariant(arr[index]);
}

//---------------------------------------------------------------------------
static svtkVariant svtkGroupLeafVerticesGetVariant(svtkAbstractArray* arr, svtkIdType i)
{
  svtkVariant val;
  switch (arr->GetDataType())
  {
    svtkSuperExtraExtendedTemplateMacro(
      val = svtkGroupLeafVerticesGetValue(static_cast<SVTK_TT*>(arr->GetVoidPointer(0)), i));
  }
  return val;
}

svtkGroupLeafVertices::svtkGroupLeafVertices()
{
  this->GroupDomain = nullptr;
  this->SetGroupDomain("group_vertex");
}

svtkGroupLeafVertices::~svtkGroupLeafVertices()
{
  this->SetGroupDomain(nullptr);
}

void svtkGroupLeafVertices::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GroupDomain: " << (this->GroupDomain ? this->GroupDomain : "(null)") << endl;
}

int svtkGroupLeafVertices::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTable and outputTree handles
  svtkTree* input = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* output = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check for corner case of 'empty' tree
  if (input->GetNumberOfVertices() == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Create builder to extend the tree
  svtkSmartPointer<svtkMutableDirectedGraph> builder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();

  // Get the input and builder vertex and edge data.
  svtkDataSetAttributes* inputVertexData = input->GetVertexData();
  svtkDataSetAttributes* inputEdgeData = input->GetEdgeData();
  svtkDataSetAttributes* builderVertexData = builder->GetVertexData();
  svtkDataSetAttributes* builderEdgeData = builder->GetEdgeData();
  builderVertexData->CopyAllocate(inputVertexData);
  builderEdgeData->CopyAllocate(inputEdgeData);

  // Get the field to filter on
  svtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr == nullptr)
  {
    svtkErrorMacro(<< "An input array must be specified");
    return 0;
  }

  // Get the builder's group array.
  char* groupname = arr->GetName();
  svtkAbstractArray* outputGroupArr = builderVertexData->GetAbstractArray(groupname);
  if (outputGroupArr == nullptr)
  {
    svtkErrorMacro(<< "Could not find the group array in the builder.");
    return 0;
  }

  // Get the (optional) name field.  Right now this will cause a warning
  // if the array is not set.
  svtkAbstractArray* inputNameArr = this->GetInputAbstractArrayToProcess(1, inputVector);

  // Get the builder's name array.
  svtkAbstractArray* outputNameArr = nullptr;
  if (inputNameArr)
  {
    char* name = inputNameArr->GetName();
    outputNameArr = builderVertexData->GetAbstractArray(name);
    if (outputNameArr == nullptr)
    {
      svtkErrorMacro(<< "Could not find the name array in the builder.");
      return 0;
    }
  }

  // Get the pedigree id array on the vertices
  svtkAbstractArray* pedigreeIdArr = builderVertexData->GetPedigreeIds();
  if (!pedigreeIdArr)
  {
    svtkErrorMacro(<< "Pedigree ids not assigned to vertices on input graph.");
    return 0;
  }

  // Get the domain array. If none exists, create one, and initialize
  bool addInputDomain = false;
  svtkStringArray* domainArr =
    svtkArrayDownCast<svtkStringArray>(builderVertexData->GetAbstractArray("domain"));
  int group_index = 0;
  if (!domainArr)
  {
    domainArr = svtkStringArray::New();
    domainArr->SetNumberOfTuples(builderVertexData->GetNumberOfTuples());
    domainArr->SetName("domain");
    builderVertexData->AddArray(domainArr);
    domainArr->Delete();
    addInputDomain = true;
  }
  else
  {
    // If a domain array already exists, look for indices that match the group
    // domain name. Use to index in to pedigree id array and find max group value.

    svtkSmartPointer<svtkIdList> groupIds = svtkSmartPointer<svtkIdList>::New();
    domainArr->LookupValue(this->GroupDomain, groupIds);

    if (pedigreeIdArr->IsNumeric())
    {
      for (svtkIdType i = 0; i < groupIds->GetNumberOfIds(); ++i)
      {
        svtkVariant v = pedigreeIdArr->GetVariantValue(i);
        bool ok;
        int num = v.ToInt(&ok);
        if (ok)
        {
          group_index = (num > group_index) ? num : group_index;
        }
      }
    }
    else if (svtkArrayDownCast<svtkStringArray>(pedigreeIdArr) ||
      svtkArrayDownCast<svtkVariantArray>(pedigreeIdArr))
    {
      for (svtkIdType i = 0; i < groupIds->GetNumberOfIds(); ++i)
      {
        std::vector<svtkStdString> tokens;
        svtkVariant v = pedigreeIdArr->GetVariantValue(i);
        splitString(v.ToString(), tokens);
        svtkVariant last = tokens[tokens.size() - 1];
        bool ok;
        int num = last.ToInt(&ok);
        if (ok)
        {
          group_index = (num > group_index) ? num : group_index;
        }
      }
    }
    else
    {
      svtkErrorMacro(<< "PedigreeId array type not supported.");
      return 0;
    }
  }

  // Copy everything into the new tree, adding group nodes.
  // Make a map of (parent id, group-by string) -> group vertex id.
  std::map<std::pair<svtkIdType, svtkVariant>, svtkIdType, svtkGroupLeafVerticesCompare> group_vertices;
  std::vector<std::pair<svtkIdType, svtkIdType> > vertStack;
  vertStack.push_back(std::make_pair(input->GetRoot(), builder->AddVertex()));
  svtkSmartPointer<svtkOutEdgeIterator> it = svtkSmartPointer<svtkOutEdgeIterator>::New();

  while (!vertStack.empty())
  {
    svtkIdType tree_v = vertStack.back().first;
    svtkIdType v = vertStack.back().second;
    builderVertexData->CopyData(inputVertexData, tree_v, v);
    vertStack.pop_back();
    input->GetOutEdges(tree_v, it);
    while (it->HasNext())
    {
      svtkOutEdgeType tree_e = it->Next();
      svtkIdType tree_child = tree_e.Target;
      svtkIdType child = builder->AddVertex();

      // If the input vertices do not have a "domain" attribute,
      // we need to set one.
      if (addInputDomain)
      {
        domainArr->InsertValue(child, pedigreeIdArr->GetName());
      }

      if (!input->IsLeaf(tree_child))
      {
        // If it isn't a leaf, just add the child to the new tree
        // and recurse.
        svtkEdgeType e = builder->AddEdge(v, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(std::make_pair(tree_child, child));
      }
      else
      {
        // If it is a leaf, it should be grouped.
        // Look for a group vertex.  If there isn't one already, make one.
        svtkIdType group_vertex = -1;
        svtkVariant groupVal = svtkGroupLeafVerticesGetVariant(arr, tree_child);
        if (group_vertices.count(std::make_pair(v, groupVal)) > 0)
        {
          group_vertex = group_vertices[std::make_pair(v, groupVal)];
        }
        else
        {
          group_vertex = builder->AddVertex();

          // Set the domain for this non-leaf vertex
          domainArr->InsertValue(group_vertex, this->GroupDomain);

          // Initialize vertex attributes that aren't the pedigree ids
          // to -1, empty string, etc.
          svtkIdType ncol = builderVertexData->GetNumberOfArrays();
          for (svtkIdType i = 0; i < ncol; i++)
          {
            svtkAbstractArray* arr2 = builderVertexData->GetAbstractArray(i);
            if (arr2 == pedigreeIdArr || arr2 == domainArr)
            {
              continue;
            }
            int comps = arr->GetNumberOfComponents();
            if (svtkArrayDownCast<svtkDataArray>(arr2))
            {
              svtkDataArray* data = svtkArrayDownCast<svtkDataArray>(arr2);
              double* tuple = new double[comps];
              for (int j = 0; j < comps; j++)
              {
                tuple[j] = -1;
              }
              data->InsertTuple(group_vertex, tuple);
              delete[] tuple;
            }
            else if (svtkArrayDownCast<svtkStringArray>(arr2))
            {
              svtkStringArray* data = svtkArrayDownCast<svtkStringArray>(arr2);
              for (int j = 0; j < comps; j++)
              {
                data->InsertValue(group_vertex + j - 1, svtkStdString(""));
              }
            }
            else if (svtkArrayDownCast<svtkVariantArray>(arr2))
            {
              svtkVariantArray* data = svtkArrayDownCast<svtkVariantArray>(arr2);
              for (int j = 0; j < comps; j++)
              {
                data->InsertValue(group_vertex + j - 1, svtkVariant());
              }
            }
            else if (svtkArrayDownCast<svtkUnicodeStringArray>(arr2))
            {
              svtkUnicodeStringArray* data = svtkArrayDownCast<svtkUnicodeStringArray>(arr2);
              for (int j = 0; j < comps; j++)
              {
                data->InsertValue(group_vertex + j - 1, svtkUnicodeString::from_utf8(""));
              }
            }
            else
            {
              svtkErrorMacro(<< "Unsupported array type for InsertNextBlankRow");
            }
          }

          svtkEdgeType group_e = builder->AddEdge(v, group_vertex);
          builderEdgeData->CopyData(inputEdgeData, tree_e.Id, group_e.Id);
          group_vertices[std::make_pair(v, groupVal)] = group_vertex;

          if (outputNameArr)
          {
            outputNameArr->InsertVariantValue(group_vertex, groupVal);
          }
          if (outputGroupArr)
          {
            outputGroupArr->InsertVariantValue(group_vertex, groupVal);
          }
          if (pedigreeIdArr != outputNameArr && pedigreeIdArr != outputGroupArr)
          {
            if (pedigreeIdArr->IsNumeric())
            {
              pedigreeIdArr->InsertVariantValue(group_vertex, group_index);
            }
            else
            {
              svtkStdString groupPrefix = "group ";
              groupPrefix += svtkVariant(group_index).ToString();
              pedigreeIdArr->InsertVariantValue(group_vertex, groupPrefix);
            }
            group_index++;
          }
        }
        svtkEdgeType e = builder->AddEdge(group_vertex, child);
        builderEdgeData->CopyData(inputEdgeData, tree_e.Id, e.Id);
        vertStack.push_back(std::make_pair(tree_child, child));
      }
    }
  }

  // Move the structure to the output
  if (!output->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid tree structure!");
    return 0;
  }

  return 1;
}

// ----------------------------------------------------------------------

static int splitString(const svtkStdString& input, std::vector<svtkStdString>& results)
{
  if (input.empty())
  {
    return 0;
  }

  char thisCharacter = 0;
  char lastCharacter = 0;

  std::string currentField;

  for (unsigned int i = 0; i < input.size(); ++i)
  {
    thisCharacter = input[i];

    // Zeroth: are we in an escape sequence? If so, interpret this
    // character accordingly.
    if (lastCharacter == '\\')
    {
      char characterToAppend;
      switch (thisCharacter)
      {
        case '0':
          characterToAppend = '\0';
          break;
        case 'a':
          characterToAppend = '\a';
          break;
        case 'b':
          characterToAppend = '\b';
          break;
        case 't':
          characterToAppend = '\t';
          break;
        case 'n':
          characterToAppend = '\n';
          break;
        case 'v':
          characterToAppend = '\v';
          break;
        case 'f':
          characterToAppend = '\f';
          break;
        case 'r':
          characterToAppend = '\r';
          break;
        case '\\':
          characterToAppend = '\\';
          break;
        default:
          characterToAppend = thisCharacter;
          break;
      }

      currentField += characterToAppend;
      lastCharacter = thisCharacter;
      if (lastCharacter == '\\')
        lastCharacter = 0;
    }
    else
    {
      // We're not in an escape sequence.

      // First, are we /starting/ an escape sequence?
      if (thisCharacter == '\\')
      {
        lastCharacter = thisCharacter;
        continue;
      }
      else if ((strchr(" ", thisCharacter) != nullptr))
      {
        // A delimiter starts a new field unless we're in a string, in
        // which case it's normal text and we won't even get here.
        if (!currentField.empty())
        {
          results.push_back(currentField);
        }
        currentField = svtkStdString();
      }
      else
      {
        // The character is just plain text.  Accumulate it and move on.
        currentField += thisCharacter;
      }

      lastCharacter = thisCharacter;
    }
  }

  results.push_back(currentField);
  return static_cast<int>(results.size());
}
