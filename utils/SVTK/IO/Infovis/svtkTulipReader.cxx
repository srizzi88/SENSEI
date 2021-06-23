/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTulipReader.cxx

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

#include "svtkTulipReader.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkVariantArray.h"
#include "svtksys/FStream.hxx"

#include <cassert>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/svtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(std::istream& stream, svtkStdString& output, char delim = '\n');

svtkStandardNewMacro(svtkTulipReader);

svtkTulipReader::svtkTulipReader()
{
  // Default values for the origin vertex
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

svtkTulipReader::~svtkTulipReader()
{
  this->SetFileName(nullptr);
}

void svtkTulipReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
}

int svtkTulipReader::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUndirectedGraph");
  }
  else if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkAnnotationLayers");
  }
  else
  {
    return 0;
  }
  return 1;
}

struct svtkTulipReaderCluster
{
  int clusterId;
  int parentId;
  static const int NO_PARENT = -1;
  svtkStdString name;
  svtkSmartPointer<svtkIdTypeArray> nodes;
};

struct svtkTulipReaderToken
{
  enum
  {
    OPEN_PAREN,
    CLOSE_PAREN,
    KEYWORD,
    INT,
    DOUBLE,
    TEXT,
    END_OF_FILE
  };
  int Type;
  svtkStdString StringValue;
  int IntValue;
  double DoubleValue;
};

static void svtkTulipReaderNextToken(std::istream& in, svtkTulipReaderToken& tok)
{
  char ch = in.peek();
  while (!in.eof() && (ch == ';' || isspace(ch)))
  {
    while (!in.eof() && ch == ';')
    {
      svtkStdString comment;
      my_getline(in, comment);
      ch = in.peek();
    }
    while (!in.eof() && isspace(ch))
    {
      in.get();
      ch = in.peek();
    }
  }

  if (in.eof())
  {
    tok.Type = svtkTulipReaderToken::END_OF_FILE;
    return;
  }
  if (ch == '(')
  {
    in.get();
    tok.Type = svtkTulipReaderToken::OPEN_PAREN;
  }
  else if (ch == ')')
  {
    in.get();
    tok.Type = svtkTulipReaderToken::CLOSE_PAREN;
  }
  else if (isdigit(ch) || ch == '.')
  {
    bool isDouble = false;
    std::stringstream ss;
    while (isdigit(ch) || ch == '.')
    {
      in.get();
      isDouble = isDouble || ch == '.';
      ss << ch;
      ch = in.peek();
    }
    if (isDouble)
    {
      ss >> tok.DoubleValue;
      tok.Type = svtkTulipReaderToken::DOUBLE;
    }
    else
    {
      ss >> tok.IntValue;
      tok.Type = svtkTulipReaderToken::INT;
    }
  }
  else if (ch == '"')
  {
    in.get();
    tok.StringValue = "";
    ch = in.get();
    while (ch != '"')
    {
      tok.StringValue += ch;
      ch = in.get();
    }
    tok.Type = svtkTulipReaderToken::TEXT;
  }
  else
  {
    in >> tok.StringValue;
    tok.Type = svtkTulipReaderToken::KEYWORD;
  }
}

int svtkTulipReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (this->FileName == nullptr)
  {
    svtkErrorMacro("File name undefined");
    return 0;
  }

  svtksys::ifstream fin(this->FileName);
  if (!fin.is_open())
  {
    svtkErrorMacro("Could not open file " << this->FileName << ".");
    return 0;
  }

  // Get the output graph
  svtkSmartPointer<svtkMutableUndirectedGraph> builder =
    svtkSmartPointer<svtkMutableUndirectedGraph>::New();

  // An array for vertex pedigree ids.
  svtkVariantArray* vertexPedigrees = svtkVariantArray::New();
  vertexPedigrees->SetName("id");
  builder->GetVertexData()->SetPedigreeIds(vertexPedigrees);
  vertexPedigrees->Delete();

  // An array for edge ids.
  svtkVariantArray* edgePedigrees = svtkVariantArray::New();
  edgePedigrees->SetName("id");

  // Structures to record cluster hierarchy - all vertices belong to cluster 0.
  std::vector<svtkTulipReaderCluster> clusters;
  svtkTulipReaderCluster root;
  root.clusterId = 0;
  root.parentId = svtkTulipReaderCluster::NO_PARENT;
  root.name = "<default>";
  root.nodes = svtkSmartPointer<svtkIdTypeArray>::New();

  std::stack<int> parentage;
  parentage.push(root.clusterId);
  clusters.push_back(root);

  std::map<int, svtkIdType> nodeIdMap;
  std::map<int, svtkIdType> edgeIdMap;
  svtkTulipReaderToken tok;
  svtkTulipReaderNextToken(fin, tok);
  while (tok.Type == svtkTulipReaderToken::OPEN_PAREN)
  {
    svtkTulipReaderNextToken(fin, tok);
    assert(tok.Type == svtkTulipReaderToken::KEYWORD);
    if (tok.StringValue == "nodes")
    {
      svtkTulipReaderNextToken(fin, tok);
      while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == svtkTulipReaderToken::INT);
        svtkIdType id = builder->AddVertex(tok.IntValue);
        nodeIdMap[tok.IntValue] = id;
        svtkTulipReaderNextToken(fin, tok);
      }
    }
    else if (tok.StringValue == "edge")
    {
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::INT);
      int tulipId = tok.IntValue;
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::INT);
      int source = tok.IntValue;
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::INT);
      int target = tok.IntValue;

      svtkEdgeType e = builder->AddEdge(nodeIdMap[source], nodeIdMap[target]);
      edgeIdMap[tulipId] = e.Id;
      edgePedigrees->InsertValue(e.Id, tulipId);

      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::CLOSE_PAREN);
    }
    else if (tok.StringValue == "cluster")
    {
      // Cluster preamble
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::INT);
      int clusterId = tok.IntValue;
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::TEXT);
      svtkStdString clusterName = tok.StringValue;

      svtkTulipReaderCluster newCluster;
      newCluster.clusterId = clusterId;
      newCluster.parentId = parentage.top();
      newCluster.name = clusterName;
      newCluster.nodes = svtkSmartPointer<svtkIdTypeArray>::New();
      parentage.push(clusterId);

      // Cluster nodes
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);

      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::KEYWORD);
      assert(tok.StringValue == "nodes");

      svtkTulipReaderNextToken(fin, tok);
      while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == svtkTulipReaderToken::INT);
        newCluster.nodes->InsertNextValue(nodeIdMap[tok.IntValue]);
        svtkTulipReaderNextToken(fin, tok);
      }

      // Cluster edges - currently ignoring these...
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);

      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::KEYWORD);
      assert(tok.StringValue == "edges");

      svtkTulipReaderNextToken(fin, tok);
      while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == svtkTulipReaderToken::INT);
        svtkTulipReaderNextToken(fin, tok);
      }
      clusters.push_back(newCluster);

      // End of cluster(s)
      svtkTulipReaderNextToken(fin, tok);
      while (tok.Type == svtkTulipReaderToken::CLOSE_PAREN)
      {
        parentage.pop();
        svtkTulipReaderNextToken(fin, tok);
      }
      continue;
    }
    else if (tok.StringValue == "property")
    {
      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::INT);
      // int clusterId = tok.IntValue;
      // We only read properties for cluster 0 (the whole graph).

      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::KEYWORD);
      svtkStdString type = tok.StringValue;

      svtkTulipReaderNextToken(fin, tok);
      assert(tok.Type == svtkTulipReaderToken::TEXT);
      svtkStdString name = tok.StringValue;

      // The existing types are the following
      // bool : This type is used to store boolean on elements.
      // color : This type is used to store the color of elements.
      //   The color is defined with a sequence of four integer from 0 to 255.
      //   "(red,green,blue,alpha)"
      // double : This is used to store 64 bits real on elements.
      // layout : This type is used to store 3D nodes position.
      //   The position of nodes is defined by a set of 3 doubles
      //   "(x_coord,y_coord,z_coord)".
      //   The position of edges is a list of 3D points.
      //   These points are the bends of edges.
      //   "((x_coord1,y_coord1,z_coord1)(x_coord2,y_coord2,z_coord2))"
      // int : This type is used to store integers on elements.
      // size : This type is used to store the size of elements.
      //   The size is defined with a sequence of three double.
      //   "(width,height,depth)"
      // string : This is used to store text on elements.

      if (type == "string")
      {
        svtkStringArray* vertArr = svtkStringArray::New();
        vertArr->SetName(name.c_str());

        svtkStringArray* edgeArr = svtkStringArray::New();
        edgeArr->SetName(name.c_str());

        svtkTulipReaderNextToken(fin, tok);
        while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::KEYWORD);
          svtkStdString key = tok.StringValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT || tok.Type == svtkTulipReaderToken::INT);
          int id = tok.IntValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT);
          svtkStdString value = tok.StringValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::CLOSE_PAREN);
          svtkTulipReaderNextToken(fin, tok);

          if (key == "node")
          {
            vertArr->InsertValue(nodeIdMap[id], value);
          }
          else if (key == "edge")
          {
            edgeArr->InsertValue(edgeIdMap[id], value);
          }
        }

        if (static_cast<size_t>(vertArr->GetNumberOfValues()) == nodeIdMap.size())
        {
          builder->GetVertexData()->AddArray(vertArr);
        }
        vertArr->Delete();
        if (static_cast<size_t>(edgeArr->GetNumberOfValues()) == edgeIdMap.size())
        {
          builder->GetEdgeData()->AddArray(edgeArr);
        }
        edgeArr->Delete();
      }
      else if (type == "int")
      {
        svtkIntArray* vertArr = svtkIntArray::New();
        vertArr->SetName(name.c_str());

        svtkIntArray* edgeArr = svtkIntArray::New();
        edgeArr->SetName(name.c_str());

        svtkTulipReaderNextToken(fin, tok);
        while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::KEYWORD);
          svtkStdString key = tok.StringValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT || tok.Type == svtkTulipReaderToken::INT);
          int id = tok.IntValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT);
          std::stringstream ss;
          int value;
          ss << tok.StringValue;
          ss >> value;
          assert(!ss.fail());
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::CLOSE_PAREN);
          svtkTulipReaderNextToken(fin, tok);

          if (key == "node")
          {
            vertArr->InsertValue(nodeIdMap[id], value);
          }
          else if (key == "edge")
          {
            edgeArr->InsertValue(edgeIdMap[id], value);
          }
        }

        if (static_cast<size_t>(vertArr->GetNumberOfTuples()) == nodeIdMap.size())
        {
          builder->GetVertexData()->AddArray(vertArr);
        }
        vertArr->Delete();
        if (static_cast<size_t>(edgeArr->GetNumberOfTuples()) == edgeIdMap.size())
        {
          builder->GetEdgeData()->AddArray(edgeArr);
        }
        edgeArr->Delete();
      }
      else if (type == "double")
      {
        svtkDoubleArray* vertArr = svtkDoubleArray::New();
        vertArr->SetName(name.c_str());

        svtkDoubleArray* edgeArr = svtkDoubleArray::New();
        edgeArr->SetName(name.c_str());

        svtkTulipReaderNextToken(fin, tok);
        while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::KEYWORD);
          svtkStdString key = tok.StringValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT || tok.Type == svtkTulipReaderToken::INT);
          int id = tok.IntValue;
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT);
          std::stringstream ss;
          double value;
          ss << tok.StringValue;
          ss >> value;
          assert(!ss.fail());
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::CLOSE_PAREN);
          svtkTulipReaderNextToken(fin, tok);

          if (key == "node")
          {
            vertArr->InsertValue(nodeIdMap[id], value);
          }
          else if (key == "edge")
          {
            edgeArr->InsertValue(edgeIdMap[id], value);
          }
        }

        if (static_cast<size_t>(vertArr->GetNumberOfTuples()) == nodeIdMap.size())
        {
          builder->GetVertexData()->AddArray(vertArr);
        }
        vertArr->Delete();
        if (static_cast<size_t>(edgeArr->GetNumberOfTuples()) == edgeIdMap.size())
        {
          builder->GetEdgeData()->AddArray(edgeArr);
        }
        edgeArr->Delete();
      }
      else // Remaining properties are ignored.
      {
        svtkTulipReaderNextToken(fin, tok);
        while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
        {
          assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::KEYWORD);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT || tok.Type == svtkTulipReaderToken::INT);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::TEXT);
          svtkTulipReaderNextToken(fin, tok);
          assert(tok.Type == svtkTulipReaderToken::CLOSE_PAREN);
          svtkTulipReaderNextToken(fin, tok);
        }
      }
    }
    else if (tok.StringValue == "displaying")
    {
      svtkTulipReaderNextToken(fin, tok);
      while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
      {
        assert(tok.Type == svtkTulipReaderToken::OPEN_PAREN);
        while (tok.Type != svtkTulipReaderToken::CLOSE_PAREN)
        {
          svtkTulipReaderNextToken(fin, tok);
        }
        svtkTulipReaderNextToken(fin, tok);
      }
    }

    svtkTulipReaderNextToken(fin, tok);
  }
  assert(parentage.size() == 1);

  // Clean up
  fin.close();

  builder->GetEdgeData()->SetPedigreeIds(edgePedigrees);
  edgePedigrees->Delete();

  // Move graph structure to output
  svtkGraph* output = svtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid graph structure.");
    return 0;
  }

  // Create annotation layers output.
  svtkSmartPointer<svtkAnnotationLayers> annotationLayers =
    svtkSmartPointer<svtkAnnotationLayers>::New();

  // Determine list of unique cluster names.
  std::set<svtkStdString> uniqueLabels;
  for (size_t i = 0; i < clusters.size(); ++i)
  {
    uniqueLabels.insert(clusters.at(i).name);
  }

  // Create annotations.
  std::set<svtkStdString>::iterator labels = uniqueLabels.begin();
  for (; labels != uniqueLabels.end(); ++labels)
  {
    svtkSmartPointer<svtkAnnotation> annotation = svtkSmartPointer<svtkAnnotation>::New();
    annotation->GetInformation()->Set(svtkAnnotation::COLOR(), 0.0, 0.0, 1.0);
    annotation->GetInformation()->Set(svtkAnnotation::OPACITY(), 0.5);
    annotation->GetInformation()->Set(svtkAnnotation::LABEL(), labels->c_str());
    annotation->GetInformation()->Set(svtkAnnotation::ENABLE(), 1);

    svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
    for (size_t i = 0; i < clusters.size(); ++i)
    {
      if (clusters.at(i).name.compare(labels->c_str()) == 0)
      {
        svtkSelectionNode* selectionNode = svtkSelectionNode::New();
        selectionNode->SetFieldType(svtkSelectionNode::VERTEX);
        selectionNode->SetContentType(svtkSelectionNode::INDICES);
        selectionNode->SetSelectionList(clusters.at(i).nodes);
        selection->AddNode(selectionNode);
        selectionNode->Delete();
      }
    }
    annotation->SetSelection(selection);
    annotationLayers->AddAnnotation(annotation);
  }

  // Copy annotations to output port 1
  svtkInformation* info1 = outputVector->GetInformationObject(1);
  svtkAnnotationLayers* output1 = svtkAnnotationLayers::GetData(info1);
  output1->ShallowCopy(annotationLayers);

  return 1;
}

static int my_getline(std::istream& in, svtkStdString& out, char delimiter)
{
  out = svtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF && numCharactersRead < out.max_size())
  {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter)
    {
      out += downcast;
    }
    else
    {
      return numCharactersRead;
    }
  }

  return numCharactersRead;
}
