/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLTreeReader.cxx

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

#include "svtkXMLTreeReader.h"

#include "svtkBitArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"

#include "svtk_libxml2.h"
#include SVTKLIBXML2_HEADER(parser.h)
#include SVTKLIBXML2_HEADER(tree.h)

svtkStandardNewMacro(svtkXMLTreeReader);

const char* svtkXMLTreeReader::TagNameField = ".tagname";
const char* svtkXMLTreeReader::CharDataField = ".chardata";

svtkXMLTreeReader::svtkXMLTreeReader()
{
  this->FileName = nullptr;
  this->XMLString = nullptr;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->ReadCharData = 0;
  this->ReadTagName = 1;
  this->MaskArrays = 0;
  this->EdgePedigreeIdArrayName = nullptr;
  this->SetEdgePedigreeIdArrayName("edge id");
  this->VertexPedigreeIdArrayName = nullptr;
  this->SetVertexPedigreeIdArrayName("vertex id");
  this->GenerateEdgePedigreeIds = true;
  this->GenerateVertexPedigreeIds = true;
}

svtkXMLTreeReader::~svtkXMLTreeReader()
{
  this->SetFileName(nullptr);
  this->SetXMLString(nullptr);
  this->SetEdgePedigreeIdArrayName(nullptr);
  this->SetVertexPedigreeIdArrayName(nullptr);
}

void svtkXMLTreeReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "ReadCharData: " << (this->ReadCharData ? "on" : "off") << endl;
  os << indent << "ReadTagName: " << (this->ReadTagName ? "on" : "off") << endl;
  os << indent << "MaskArrays: " << (this->MaskArrays ? "on" : "off") << endl;
  os << indent << "XMLString: " << (this->XMLString ? this->XMLString : "(none)") << endl;
  os << indent << "EdgePedigreeIdArrayName: "
     << (this->EdgePedigreeIdArrayName ? this->EdgePedigreeIdArrayName : "(null)") << endl;
  os << indent << "VertexPedigreeIdArrayName: "
     << (this->VertexPedigreeIdArrayName ? this->VertexPedigreeIdArrayName : "(null)") << endl;
  os << indent << "GenerateEdgePedigreeIds: " << (this->GenerateEdgePedigreeIds ? "on" : "off")
     << endl;
  os << indent << "GenerateVertexPedigreeIds: " << (this->GenerateVertexPedigreeIds ? "on" : "off")
     << endl;
}

static void svtkXMLTreeReaderProcessElement(
  svtkMutableDirectedGraph* tree, svtkIdType parent, xmlNode* node, int readCharData, int maskArrays)
{
  svtkDataSetAttributes* data = tree->GetVertexData();
  svtkStringArray* nameArr =
    svtkArrayDownCast<svtkStringArray>(data->GetAbstractArray(svtkXMLTreeReader::TagNameField));
  svtkStdString content;
  for (xmlNode* curNode = node; curNode; curNode = curNode->next)
  {
    // cerr << "type=" << curNode->type << ",name=" << curNode->name << endl;
    if (curNode->content)
    {
      const char* curContent = reinterpret_cast<const char*>(curNode->content);
      content += curContent;
      // cerr << "content=" << curNode->content << endl;
    }

    if (curNode->type != XML_ELEMENT_NODE)
    {
      continue;
    }

    svtkIdType vertex = tree->AddVertex();
    if (parent != -1)
    {
      tree->AddEdge(parent, vertex);
    }

    // Append the node tag and character data to the svtkPointData
    if (nameArr) // If the reader has ReadTagName = ON then populate this array
    {
      nameArr->InsertValue(vertex, reinterpret_cast<const char*>(curNode->name));
    }

    // Append the element attributes to the svtkPointData
    for (xmlAttr* curAttr = curNode->properties; curAttr; curAttr = curAttr->next)
    {
      const char* name = reinterpret_cast<const char*>(curAttr->name);
      int len = static_cast<int>(strlen(name));
      char* validName = new char[len + 8];
      strcpy(validName, ".valid.");
      strcat(validName, name);
      svtkStringArray* stringArr = svtkArrayDownCast<svtkStringArray>(data->GetAbstractArray(name));
      svtkBitArray* bitArr = nullptr;
      if (maskArrays)
      {
        bitArr = svtkArrayDownCast<svtkBitArray>(data->GetAbstractArray(validName));
      }
      if (!stringArr)
      {
        stringArr = svtkStringArray::New();
        stringArr->SetName(name);
        data->AddArray(stringArr);
        stringArr->Delete();
        if (maskArrays)
        {
          bitArr = svtkBitArray::New();
          bitArr->SetName(validName);
          data->AddArray(bitArr);
          bitArr->Delete();
        }
      }
      const char* value = reinterpret_cast<const char*>(curAttr->children->content);
      stringArr->InsertValue(vertex, value);
      if (maskArrays)
      {
        for (svtkIdType i = bitArr->GetNumberOfTuples(); i < vertex; i++)
        {
          bitArr->InsertNextValue(false);
        }
        bitArr->InsertNextValue(true);
      }
      // cerr << "attname=" << name << ",value=" << value << endl;
      delete[] validName;
    }

    // Process this node's children
    svtkXMLTreeReaderProcessElement(tree, vertex, curNode->children, readCharData, maskArrays);
  }

  if (readCharData && parent >= 0)
  {
    svtkStringArray* charArr =
      svtkArrayDownCast<svtkStringArray>(data->GetAbstractArray(svtkXMLTreeReader::CharDataField));
    charArr->InsertValue(parent, content);
  }
}

int svtkXMLTreeReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (!this->FileName && !this->XMLString)
  {
    svtkErrorMacro("A FileName or XMLString must be specified");
    return 0;
  }

  xmlDoc* doc = nullptr;
  if (this->FileName)
  {
    // Parse the file and get the DOM
    doc = xmlReadFile(this->FileName, nullptr, 0);
  }
  else if (this->XMLString)
  {
    // Parse from memory and get the DOM
    doc = xmlReadMemory(
      this->XMLString, static_cast<int>(strlen(this->XMLString)), "noname.xml", nullptr, 0);
  }

  // Store the XML hierarchy into a svtkMutableDirectedGraph,
  // later to be placed in a svtkTree.
  svtkSmartPointer<svtkMutableDirectedGraph> builder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();

  svtkDataSetAttributes* data = builder->GetVertexData();

  if (this->ReadTagName)
  {
    svtkStringArray* nameArr = svtkStringArray::New();
    nameArr->SetName(svtkXMLTreeReader::TagNameField);
    data->AddArray(nameArr);
    nameArr->Delete();
  }

  if (this->ReadCharData)
  {
    svtkStringArray* charArr = svtkStringArray::New();
    charArr->SetName(svtkXMLTreeReader::CharDataField);
    data->AddArray(charArr);
    charArr->Delete();
  }

  // Get the root element node
  xmlNode* rootElement = xmlDocGetRootElement(doc);
  svtkXMLTreeReaderProcessElement(builder, -1, rootElement, this->ReadCharData, this->MaskArrays);

  xmlFreeDoc(doc);

  // Make all the arrays the same size
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    svtkStringArray* stringArr = svtkArrayDownCast<svtkStringArray>(data->GetAbstractArray(i));
    if (stringArr && (stringArr->GetNumberOfTuples() < builder->GetNumberOfVertices()))
    {
      stringArr->InsertValue(builder->GetNumberOfVertices() - 1, svtkStdString(""));
    }
  }

  // Move the XML hierarchy into a svtkTree
  svtkTree* output = svtkTree::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Structure is not a valid tree!");
    return 0;
  }

  // Look for or generate vertex pedigree id array.
  if (this->GenerateVertexPedigreeIds)
  {
    // Create vertex pedigree ids
    svtkSmartPointer<svtkIdTypeArray> ids = svtkSmartPointer<svtkIdTypeArray>::New();
    ids->SetName(this->VertexPedigreeIdArrayName);
    svtkIdType numVerts = output->GetNumberOfVertices();
    ids->SetNumberOfTuples(numVerts);
    for (svtkIdType i = 0; i < numVerts; ++i)
    {
      ids->SetValue(i, i);
    }
    output->GetVertexData()->SetPedigreeIds(ids);
  }
  else
  {
    svtkAbstractArray* pedIds =
      output->GetVertexData()->GetAbstractArray(this->VertexPedigreeIdArrayName);
    if (pedIds)
    {
      output->GetVertexData()->SetPedigreeIds(pedIds);
    }
    else
    {
      svtkErrorMacro(<< "Vertex pedigree ID array not found.");
      return 0;
    }
  }

  // Look for or generate edge pedigree id array.
  if (this->GenerateEdgePedigreeIds)
  {
    // Create vertex pedigree ids
    svtkSmartPointer<svtkIdTypeArray> ids = svtkSmartPointer<svtkIdTypeArray>::New();
    ids->SetName(this->EdgePedigreeIdArrayName);
    svtkIdType numEdges = output->GetNumberOfEdges();
    ids->SetNumberOfTuples(numEdges);
    for (svtkIdType i = 0; i < numEdges; ++i)
    {
      ids->SetValue(i, i);
    }
    output->GetEdgeData()->SetPedigreeIds(ids);
  }
  else
  {
    svtkAbstractArray* pedIds =
      output->GetEdgeData()->GetAbstractArray(this->EdgePedigreeIdArrayName);
    if (pedIds)
    {
      output->GetEdgeData()->SetPedigreeIds(pedIds);
    }
    else
    {
      svtkErrorMacro(<< "Edge pedigree ID array not found.");
      return 0;
    }
  }

  return 1;
}
