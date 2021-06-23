/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGraphReader.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkGraphReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
svtkGraphReader::svtkGraphReader() = default;

//----------------------------------------------------------------------------
svtkGraphReader::~svtkGraphReader() = default;

//----------------------------------------------------------------------------
svtkGraph* svtkGraphReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkGraph* svtkGraphReader::GetOutput(int idx)
{
  return svtkGraph::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
int svtkGraphReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkDebugMacro(<< "Reading svtk graph ...");
  char line[256];

  GraphType graphType;
  if (!this->ReadGraphType(fname.c_str(), graphType))
  {
    this->CloseSVTKFile();
    return 1;
  }

  svtkSmartPointer<svtkMutableDirectedGraph> dir_builder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();
  svtkSmartPointer<svtkMutableUndirectedGraph> undir_builder =
    svtkSmartPointer<svtkMutableUndirectedGraph>::New();

  svtkGraph* builder = nullptr;
  switch (graphType)
  {
    case svtkGraphReader::DirectedGraph:
      builder = dir_builder;
      break;

    case svtkGraphReader::UndirectedGraph:
    case svtkGraphReader::Molecule: // Extends undirected graph.
      builder = undir_builder;
      break;

    case svtkGraphReader::UnknownGraph:
    default:
      svtkErrorMacro("ReadGraphType gave invalid result.");
      this->CloseSVTKFile();
      return 1;
  }

  // Lattice information for molecules:
  bool hasLattice = false;
  svtkVector3d lattice_a;
  svtkVector3d lattice_b;
  svtkVector3d lattice_c;
  svtkVector3d lattice_origin;

  while (true)
  {
    if (!this->ReadString(line))
    {
      break;
    }

    if (!strncmp(this->LowerCase(line), "field", 5))
    {
      svtkFieldData* const field_data = this->ReadFieldData();
      switch (graphType)
      {
        case svtkGraphReader::DirectedGraph:
          dir_builder->SetFieldData(field_data);
          break;

        case svtkGraphReader::UndirectedGraph:
        case svtkGraphReader::Molecule:
          undir_builder->SetFieldData(field_data);
          break;

        case svtkGraphReader::UnknownGraph:
        default: // Can't happen, would return earlier.
          break;
      }

      field_data->Delete();
      continue;
    }

    if (!strncmp(this->LowerCase(line), "points", 6))
    {
      svtkIdType point_count = 0;
      if (!this->Read(&point_count))
      {
        svtkErrorMacro(<< "Cannot read number of points!");
        this->CloseSVTKFile();
        return 1;
      }

      this->ReadPointCoordinates(builder, point_count);
      continue;
    }

    if (!strncmp(this->LowerCase(line), "vertices", 8))
    {
      svtkIdType vertex_count = 0;
      if (!this->Read(&vertex_count))
      {
        svtkErrorMacro(<< "Cannot read number of vertices!");
        this->CloseSVTKFile();
        return 1;
      }
      for (svtkIdType v = 0; v < vertex_count; ++v)
      {
        switch (graphType)
        {
          case svtkGraphReader::DirectedGraph:
            dir_builder->AddVertex();
            break;

          case svtkGraphReader::UndirectedGraph:
          case svtkGraphReader::Molecule:
            undir_builder->AddVertex();
            break;

          case svtkGraphReader::UnknownGraph:
          default: // Can't happen, would return earlier.
            break;
        }
      }
      continue;
    }

    if (!strncmp(this->LowerCase(line), "edges", 5))
    {
      svtkIdType edge_count = 0;
      if (!this->Read(&edge_count))
      {
        svtkErrorMacro(<< "Cannot read number of edges!");
        this->CloseSVTKFile();
        return 1;
      }
      svtkIdType source = 0;
      svtkIdType target = 0;
      for (svtkIdType edge = 0; edge != edge_count; ++edge)
      {
        if (!(this->Read(&source) && this->Read(&target)))
        {
          svtkErrorMacro(<< "Cannot read edge!");
          this->CloseSVTKFile();
          return 1;
        }

        switch (graphType)
        {
          case svtkGraphReader::DirectedGraph:
            dir_builder->AddEdge(source, target);
            break;

          case svtkGraphReader::UndirectedGraph:
          case svtkGraphReader::Molecule:
            undir_builder->AddEdge(source, target);
            break;

          case svtkGraphReader::UnknownGraph:
          default: // Can't happen, would return earlier.
            break;
        }
      }
      continue;
    }

    if (!strncmp(this->LowerCase(line), "vertex_data", 10))
    {
      svtkIdType vertex_count = 0;
      if (!this->Read(&vertex_count))
      {
        svtkErrorMacro(<< "Cannot read number of vertices!");
        this->CloseSVTKFile();
        return 1;
      }

      this->ReadVertexData(builder, vertex_count);
      continue;
    }

    if (!strncmp(this->LowerCase(line), "edge_data", 9))
    {
      svtkIdType edge_count = 0;
      if (!this->Read(&edge_count))
      {
        svtkErrorMacro(<< "Cannot read number of edges!");
        this->CloseSVTKFile();
        return 1;
      }

      this->ReadEdgeData(builder, edge_count);
      continue;
    }

    if (!strncmp(this->LowerCase(line), "lattice_", 8))
    {
      switch (line[8]) // lattice_<line[8]> -- which vector: a, b, c, or origin?
      {
        case 'a':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_a[i]))
            {
              svtkErrorMacro("Error while parsing lattice information.");
              this->CloseSVTKFile();
              return 1;
            }
          }
          continue;

        case 'b':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_b[i]))
            {
              svtkErrorMacro("Error while parsing lattice information.");
              this->CloseSVTKFile();
              return 1;
            }
          }
          continue;

        case 'c':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_c[i]))
            {
              svtkErrorMacro("Error while parsing lattice information.");
              this->CloseSVTKFile();
              return 1;
            }
          }
          continue;

        case 'o':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_origin[i]))
            {
              svtkErrorMacro("Error while parsing lattice information.");
              this->CloseSVTKFile();
              return 1;
            }
          }
          continue;

        default:
          break;
      }
    }

    svtkErrorMacro(<< "Unrecognized keyword: " << line);
  }

  svtkDebugMacro(<< "Read " << builder->GetNumberOfVertices() << " vertices and "
                << builder->GetNumberOfEdges() << " edges.\n");

  this->CloseSVTKFile();

  // Copy builder into output.
  svtkGraph* output = svtkGraph::SafeDownCast(doOutput);
  bool valid = output->CheckedShallowCopy(builder);

  svtkMolecule* mol = svtkMolecule::SafeDownCast(output);
  if (valid && hasLattice && mol)
  {
    mol->SetLattice(lattice_a, lattice_b, lattice_c);
    mol->SetLatticeOrigin(lattice_origin);
  }

  if (!valid)
  {
    svtkErrorMacro(<< "Invalid graph structure, returning empty graph.");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkGraphReader::ReadGraphType(const char* fname, GraphType& type)
{
  type = UnknownGraph;

  if (!this->OpenSVTKFile(fname) || !this->ReadHeader())
  {
    return 0;
  }

  // Read graph-specific stuff
  char line[256];
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 0;
  }

  if (strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    svtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseSVTKFile();
    return 0;
  }

  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 0;
  }

  if (!strncmp(this->LowerCase(line), "directed_graph", 14))
  {
    type = DirectedGraph;
  }
  else if (!strncmp(this->LowerCase(line), "undirected_graph", 16))
  {
    type = UndirectedGraph;
  }
  else if (!strncmp(this->LowerCase(line), "molecule", 8))
  {
    type = Molecule;
  }
  else
  {
    svtkErrorMacro(<< "Cannot read type: " << line);
    this->CloseSVTKFile();
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkGraphReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkGraphReader::CreateOutput(svtkDataObject* currentOutput)
{
  GraphType graphType;
  if (!this->ReadGraphType(this->GetFileName(), graphType))
  {
    this->CloseSVTKFile();
    return nullptr;
  }
  this->CloseSVTKFile();

  switch (graphType)
  {
    case svtkGraphReader::DirectedGraph:
      if (currentOutput && currentOutput->IsA("svtkDirectedGraph"))
      {
        return currentOutput;
      }
      return svtkDirectedGraph::New();

    case svtkGraphReader::UndirectedGraph:
      if (currentOutput && currentOutput->IsA("svtkUndirectedGraph"))
      {
        return currentOutput;
      }
      return svtkUndirectedGraph::New();

    case svtkGraphReader::Molecule:
      if (currentOutput && currentOutput->IsA("svtkMolecule"))
      {
        return currentOutput;
      }
      return svtkMolecule::New();

    case svtkGraphReader::UnknownGraph:
    default:
      svtkErrorMacro("ReadGraphType returned invalid result.");
      return nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkGraphReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
