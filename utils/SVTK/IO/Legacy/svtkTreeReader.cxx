/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTreeReader.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkTreeReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
svtkTreeReader::svtkTreeReader() = default;

//----------------------------------------------------------------------------
svtkTreeReader::~svtkTreeReader() = default;

//----------------------------------------------------------------------------
svtkTree* svtkTreeReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkTree* svtkTreeReader::GetOutput(int idx)
{
  return svtkTree::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkTreeReader::SetOutput(svtkTree* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkTreeReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkDebugMacro(<< "Reading svtk tree ...");

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader())
  {
    return 1;
  }

  // Read table-specific stuff
  char line[256];
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    svtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseSVTKFile();
    return 1;
  }

  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (strncmp(this->LowerCase(line), "tree", 4))
  {
    svtkErrorMacro(<< "Cannot read dataset type: " << line);
    this->CloseSVTKFile();
    return 1;
  }

  svtkTree* const output = svtkTree::SafeDownCast(doOutput);

  svtkSmartPointer<svtkMutableDirectedGraph> builder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();

  while (true)
  {
    if (!this->ReadString(line))
    {
      break;
    }

    if (!strncmp(this->LowerCase(line), "field", 5))
    {
      svtkFieldData* const field_data = this->ReadFieldData();
      builder->SetFieldData(field_data);
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

    if (!strncmp(this->LowerCase(line), "edges", 5))
    {
      svtkIdType edge_count = 0;
      if (!this->Read(&edge_count))
      {
        svtkErrorMacro(<< "Cannot read number of edges!");
        this->CloseSVTKFile();
        return 1;
      }

      // Create all of the tree vertices (number of edges + 1)
      for (svtkIdType edge = 0; edge <= edge_count; ++edge)
      {
        builder->AddVertex();
      }

      // Reparent the existing vertices so their order and topology match the original
      svtkIdType child = 0;
      svtkIdType parent = 0;
      for (svtkIdType edge = 0; edge != edge_count; ++edge)
      {
        if (!(this->Read(&child) && this->Read(&parent)))
        {
          svtkErrorMacro(<< "Cannot read edge!");
          this->CloseSVTKFile();
          return 1;
        }

        builder->AddEdge(parent, child);
      }

      if (!output->CheckedShallowCopy(builder))
      {
        svtkErrorMacro(<< "Edges do not create a valid tree.");
        this->CloseSVTKFile();
        return 1;
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

      this->ReadVertexData(output, vertex_count);
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

      this->ReadEdgeData(output, edge_count);
      continue;
    }

    svtkErrorMacro(<< "Unrecognized keyword: " << line);
  }

  svtkDebugMacro(<< "Read " << output->GetNumberOfVertices() << " vertices and "
                << output->GetNumberOfEdges() << " edges.\n");

  this->CloseSVTKFile();

  return 1;
}

//----------------------------------------------------------------------------
int svtkTreeReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTree");
  return 1;
}

//----------------------------------------------------------------------------
void svtkTreeReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
