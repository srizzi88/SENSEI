/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkXGMLReader.cxx

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

#include "svtkXGMLReader.h"

#include "svtkAbstractArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtksys/FStream.hxx"

#include <cassert>
#include <cctype> // for isspace, isdigit
#include <fstream>
#include <map>
#include <sstream>

// Copied from svtkTulipReader.cxx ..
static int my_getline(std::istream& stream, svtkStdString& output, char delim = '\n');

svtkStandardNewMacro(svtkXGMLReader);

svtkXGMLReader::svtkXGMLReader()
{
  // Default values for the origin vertex
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

svtkXGMLReader::~svtkXGMLReader()
{
  this->SetFileName(nullptr);
}

void svtkXGMLReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
}

#define MAX_NR_PROPERTIES 50

struct svtkXGMLProperty
{
  enum
  {
    NODE_PROP,
    EDGE_PROP
  };
  int Kind; // :: NODE_PROP or EDGE_PROP
  svtkAbstractArray* Data;
};

struct svtkXGMLReaderToken
{
  enum
  {
    OPEN_GROUP,
    CLOSE_GROUP,
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

static void svtkXGMLReaderNextToken(std::istream& in, svtkXGMLReaderToken& tok)
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
    tok.Type = svtkXGMLReaderToken::END_OF_FILE;
    return;
  }
  if (ch == '[')
  {
    in.get();
    tok.Type = svtkXGMLReaderToken::OPEN_GROUP;
  }
  else if (ch == ']')
  {
    in.get();
    tok.Type = svtkXGMLReaderToken::CLOSE_GROUP;
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
      tok.Type = svtkXGMLReaderToken::DOUBLE;
    }
    else
    {
      ss >> tok.IntValue;
      tok.Type = svtkXGMLReaderToken::INT;
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
    tok.Type = svtkXGMLReaderToken::TEXT;
  }
  else
  {
    in >> tok.StringValue;
    tok.Type = svtkXGMLReaderToken::KEYWORD;
  }
}

int svtkXGMLReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkIdType nr_of_nodes = 0; // as read from file
  svtkIdType nr_of_edges = 0; // as read from file
  int nr_of_properties = 0;
  svtkXGMLProperty property_table[MAX_NR_PROPERTIES];
  svtkStdString name;
  int kind;
  int i;
  svtkIdType dst, id = 0, src = 0;
  double d = 0.;
  svtkIdTypeArray *edgeIds, *nodeIds;

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

  std::map<int, svtkIdType> nodeIdMap;
  std::map<int, svtkIdType> edgeIdMap;
  svtkXGMLReaderToken tok;

  // expect graph
  svtkXGMLReaderNextToken(fin, tok);
  assert(tok.Type == svtkXGMLReaderToken::KEYWORD && tok.StringValue == "graph");

  // expect [
  svtkXGMLReaderNextToken(fin, tok);
  assert(tok.Type == svtkXGMLReaderToken::OPEN_GROUP);

  svtkXGMLReaderNextToken(fin, tok);
  while (tok.Type == svtkXGMLReaderToken::KEYWORD && tok.StringValue != "node")
  {
    if (tok.StringValue == "node_count")
    {
      svtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == svtkXGMLReaderToken::INT);
      nr_of_nodes = tok.IntValue;
    }
    else if (tok.StringValue == "edge_count")
    {
      svtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == svtkXGMLReaderToken::INT);
      nr_of_edges = tok.IntValue;
    }
    else if (tok.StringValue == "node_data" || tok.StringValue == "edge_data")
    {
      if (nr_of_properties == MAX_NR_PROPERTIES)
      {
        svtkErrorMacro(<< "Too many properties in file.");
        return 0;
      }
      kind =
        (tok.StringValue == "node_data") ? svtkXGMLProperty::NODE_PROP : svtkXGMLProperty::EDGE_PROP;
      svtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == svtkXGMLReaderToken::KEYWORD);
      name = tok.StringValue;

      svtkXGMLReaderNextToken(fin, tok);
      assert(tok.Type == svtkXGMLReaderToken::KEYWORD);
      if (tok.StringValue == "float")
      {
        property_table[nr_of_properties].Data = svtkDoubleArray::New();
      }
      else if (tok.StringValue == "int")
      {
        property_table[nr_of_properties].Data = svtkIntArray::New();
      }
      else if (tok.StringValue == "string")
      {
        property_table[nr_of_properties].Data = svtkStringArray::New();
      }
      property_table[nr_of_properties].Kind = kind;
      property_table[nr_of_properties].Data->SetName(name);
      property_table[nr_of_properties].Data->SetNumberOfTuples(
        kind == svtkXGMLProperty::NODE_PROP ? nr_of_nodes : nr_of_edges);
      nr_of_properties++;
    }
    else
    {
      svtkErrorMacro(<< "Parse error (header): unexpected token ");
      return 0;
    }
    svtkXGMLReaderNextToken(fin, tok);
  }

  while (tok.Type == svtkXGMLReaderToken::KEYWORD && tok.StringValue == "node")
  {
    // Expect [
    svtkXGMLReaderNextToken(fin, tok);
    assert(tok.Type == svtkXGMLReaderToken::OPEN_GROUP);

    svtkXGMLReaderNextToken(fin, tok);
    while (tok.Type == svtkXGMLReaderToken::KEYWORD)
    {
      if (tok.StringValue == "id")
      {
        svtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == svtkXGMLReaderToken::INT);
        id = builder->AddVertex();
        nodeIdMap[tok.IntValue] = id;
      }
      else if (tok.StringValue == "degree")
      {
        svtkXGMLReaderNextToken(fin, tok);
        // Read degree and ignore
      }
      else
      {
        for (i = 0; i < nr_of_properties; i++)
        {
          if (property_table[i].Kind == svtkXGMLProperty::NODE_PROP &&
            property_table[i].Data->GetName() == tok.StringValue)
          {
            break;
          }
        }
        if (i == nr_of_properties)
        {
          svtkErrorMacro(<< "Undefined node property ");
          cout << tok.StringValue << "\n";
          return 0;
        }
        svtkXGMLReaderNextToken(fin, tok);
        if (property_table[i].Data->GetDataType() == SVTK_INT)
        {
          assert(tok.Type == svtkXGMLReaderToken::INT);
          svtkArrayDownCast<svtkIntArray>(property_table[i].Data)
            ->SetValue(nodeIdMap[id], tok.IntValue);
        }
        else if (property_table[i].Data->GetDataType() == SVTK_DOUBLE)
        {
          if (tok.Type == svtkXGMLReaderToken::DOUBLE)
            d = tok.DoubleValue;
          else if (tok.Type == svtkXGMLReaderToken::INT)
            d = (double)tok.IntValue;
          else
            svtkErrorMacro(<< "Expected double or int.\n");
          svtkArrayDownCast<svtkDoubleArray>(property_table[i].Data)->SetValue(nodeIdMap[id], d);
        }
        else
        {
          assert(tok.Type == svtkXGMLReaderToken::TEXT);
          svtkArrayDownCast<svtkStringArray>(property_table[i].Data)
            ->SetValue(nodeIdMap[id], tok.StringValue);
        }
      }
      svtkXGMLReaderNextToken(fin, tok);
    }
    assert(tok.Type == svtkXGMLReaderToken::CLOSE_GROUP);
    svtkXGMLReaderNextToken(fin, tok);
  }

  while (tok.Type == svtkXGMLReaderToken::KEYWORD && tok.StringValue == "edge")
  {
    // Expect [
    svtkXGMLReaderNextToken(fin, tok);
    assert(tok.Type == svtkXGMLReaderToken::OPEN_GROUP);

    svtkXGMLReaderNextToken(fin, tok);
    while (tok.Type == svtkXGMLReaderToken::KEYWORD)
    {
      // Assume that all edge groups will list id, source, and dest fields
      // before any edge property.
      if (tok.StringValue == "id")
      {
        svtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == svtkXGMLReaderToken::INT);
        id = tok.IntValue;
      }
      else if (tok.StringValue == "source")
      {
        svtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == svtkXGMLReaderToken::INT);
        src = tok.IntValue;
      }
      else if (tok.StringValue == "target")
      {
        svtkXGMLReaderNextToken(fin, tok);
        assert(tok.Type == svtkXGMLReaderToken::INT);
        dst = tok.IntValue;
        svtkEdgeType e = builder->AddEdge(nodeIdMap[src], nodeIdMap[dst]);
        edgeIdMap[id] = e.Id;
      }
      else
      {
        for (i = 0; i < nr_of_properties; i++)
        {
          if (property_table[i].Kind == svtkXGMLProperty::EDGE_PROP &&
            property_table[i].Data->GetName() == tok.StringValue)
          {
            break;
          }
        }
        if (i == nr_of_properties)
        {
          svtkErrorMacro(<< "Undefined node property ");
          return 0;
        }
        svtkXGMLReaderNextToken(fin, tok);
        if (property_table[i].Data->GetDataType() == SVTK_INT)
        {
          assert(tok.Type == svtkXGMLReaderToken::INT);
          svtkArrayDownCast<svtkIntArray>(property_table[i].Data)
            ->SetValue(edgeIdMap[id], tok.IntValue);
        }
        else if (property_table[i].Data->GetDataType() == SVTK_DOUBLE)
        {
          if (tok.Type == svtkXGMLReaderToken::DOUBLE)
            d = tok.DoubleValue;
          else if (tok.Type == svtkXGMLReaderToken::INT)
            d = (double)tok.IntValue;
          else
            svtkErrorMacro(<< "Expected double or int.\n");
          svtkArrayDownCast<svtkDoubleArray>(property_table[i].Data)->SetValue(nodeIdMap[id], d);
        }
        else
        {
          assert(tok.Type == svtkXGMLReaderToken::TEXT);
          svtkArrayDownCast<svtkStringArray>(property_table[i].Data)
            ->SetValue(edgeIdMap[id], tok.StringValue);
        }
      }
      svtkXGMLReaderNextToken(fin, tok);
    }
    assert(tok.Type == svtkXGMLReaderToken::CLOSE_GROUP);
    svtkXGMLReaderNextToken(fin, tok);
  }

  // Should now recognise the end of graph group ..
  assert(tok.Type == svtkXGMLReaderToken::CLOSE_GROUP);

  // .. followed by end-of-file.
  svtkXGMLReaderNextToken(fin, tok);
  // do an extra read
  svtkXGMLReaderNextToken(fin, tok);
  assert(tok.Type == svtkXGMLReaderToken::END_OF_FILE);

  // Clean up
  fin.close();

  for (i = 0; i < nr_of_properties; i++)
  {
    if (property_table[i].Kind == svtkXGMLProperty::NODE_PROP)
    {
      builder->GetVertexData()->AddArray(property_table[i].Data);
      property_table[i].Data->Delete();
    }
    else
    {
      builder->GetEdgeData()->AddArray(property_table[i].Data);
      property_table[i].Data->Delete();
    }
  }
  svtkFloatArray* weights = svtkFloatArray::New();
  weights->SetName("edge weight");
  weights->SetNumberOfTuples(nr_of_edges);
  edgeIds = svtkIdTypeArray::New();
  edgeIds->SetName("edge id");
  edgeIds->SetNumberOfTuples(nr_of_edges);
  for (i = 0; i < nr_of_edges; i++)
  {
    weights->SetValue(i, 1.0);
    edgeIds->SetValue(i, i);
  }

  nodeIds = svtkIdTypeArray::New();
  nodeIds->SetName("vertex id");
  nodeIds->SetNumberOfTuples(nr_of_nodes);
  for (i = 0; i < nr_of_nodes; i++)
  {
    nodeIds->SetValue(i, i);
  }
  builder->GetEdgeData()->AddArray(weights);
  builder->GetEdgeData()->SetPedigreeIds(edgeIds);
  builder->GetVertexData()->SetPedigreeIds(nodeIds);
  weights->Delete();
  nodeIds->Delete();
  edgeIds->Delete();
  // Move structure to output
  svtkGraph* output = svtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid graph structure.");
    return 0;
  }

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
