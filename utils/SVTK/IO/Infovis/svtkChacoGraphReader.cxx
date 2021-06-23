/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChacoGraphReader.cxx

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
// .NAME svtkChacoGraphReader - Reads chaco graph files.
//
// .SECTION Description
// svtkChacoGraphReader reads in files in the CHACO format.
// An example is the following:
// <code>
// 10 13
// 2 6 10
// 1 3
// 2 4 8
// 3 5
// 4 6 10
// 1 5 7
// 6 8
// 3 7 9
// 8 10
// 1 5 9
// </code>
// The first line gives the number of vertices and edges in the graph.
// Each additional line is the connectivity list for each vertex in the graph.
// Vertex 1 is connected to 2, 6, and 10, vertex 2 is connected to 1 and 3, etc.
// NOTE: Chaco ids start with 1, while SVTK ids start at 0, so SVTK ids will be
// one less than the chaco ids.

#include "svtkChacoGraphReader.h"

#include "svtkCellData.h"
#include "svtkIntArray.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtksys/FStream.hxx"

#include <fstream>
#include <sstream>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/svtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(std::istream& stream, svtkStdString& output, char delim = '\n');

svtkStandardNewMacro(svtkChacoGraphReader);

svtkChacoGraphReader::svtkChacoGraphReader()
{
  // Default values for the origin vertex
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

svtkChacoGraphReader::~svtkChacoGraphReader()
{
  this->SetFileName(nullptr);
}

void svtkChacoGraphReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
}

int svtkChacoGraphReader::RequestData(svtkInformation* svtkNotUsed(request),
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

  // Create a mutable graph builder
  SVTK_CREATE(svtkMutableUndirectedGraph, builder);

  // Get the header line
  svtkStdString line;
  my_getline(fin, line);
  std::stringstream firstLine;
  firstLine << line;
  svtkIdType numVerts;
  svtkIdType numEdges;
  firstLine >> numVerts >> numEdges;
  svtkIdType type = 0;
  if (firstLine.good())
  {
    firstLine >> type;
  }

  // Create the weight arrays
  int vertWeights = type % 10;
  int edgeWeights = (type / 10) % 10;
  // cerr << "type=" << type << ",vertWeights=" << vertWeights << ",edgeWeights=" << edgeWeights <<
  // endl;
  svtkIntArray** vertArr = new svtkIntArray*[vertWeights];
  for (int vw = 0; vw < vertWeights; vw++)
  {
    std::ostringstream oss;
    oss << "weight " << (vw + 1);
    vertArr[vw] = svtkIntArray::New();
    vertArr[vw]->SetName(oss.str().c_str());
    builder->GetVertexData()->AddArray(vertArr[vw]);
    vertArr[vw]->Delete();
  }
  svtkIntArray** edgeArr = new svtkIntArray*[edgeWeights];
  for (int ew = 0; ew < edgeWeights; ew++)
  {
    std::ostringstream oss;
    oss << "weight " << (ew + 1);
    edgeArr[ew] = svtkIntArray::New();
    edgeArr[ew]->SetName(oss.str().c_str());
    builder->GetEdgeData()->AddArray(edgeArr[ew]);
    edgeArr[ew]->Delete();
  }

  // Add the vertices
  for (svtkIdType v = 0; v < numVerts; v++)
  {
    builder->AddVertex();
  }

  // Add the edges
  for (svtkIdType u = 0; u < numVerts; u++)
  {
    my_getline(fin, line);
    std::stringstream stream;
    stream << line;
    // cerr << "read line " << stream.str() << endl;
    int weight;
    for (int vw = 0; vw < vertWeights; vw++)
    {
      stream >> weight;
      vertArr[vw]->InsertNextValue(weight);
    }
    svtkIdType v;
    while (stream.good())
    {
      stream >> v;
      // cerr << "read adjacent vertex " << v << endl;

      // svtkGraph ids are 1 less than Chaco graph ids
      v--;
      // Only add the edge if v less than u.
      // This avoids adding the same edge twice.
      if (v < u)
      {
        builder->AddEdge(u, v);
        for (int ew = 0; ew < edgeWeights; ew++)
        {
          stream >> weight;
          edgeArr[ew]->InsertNextValue(weight);
        }
      }
    }
  }
  delete[] edgeArr;
  delete[] vertArr;

  // Clean up
  fin.close();

  // Get the output graph
  svtkGraph* output = svtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid graph structure");
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
