/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomGraphSource.cxx

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
#include "svtkRandomGraphSource.h"

#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

#include <algorithm>
#include <set>

svtkStandardNewMacro(svtkRandomGraphSource);

// ----------------------------------------------------------------------

svtkRandomGraphSource::svtkRandomGraphSource()
{
  this->NumberOfVertices = 10;
  this->NumberOfEdges = 10;
  this->EdgeProbability = 0.5;
  this->IncludeEdgeWeights = false;
  this->Directed = 0;
  this->UseEdgeProbability = 0;
  this->StartWithTree = 0;
  this->AllowSelfLoops = false;
  this->AllowParallelEdges = false;
  this->GeneratePedigreeIds = true;
  this->VertexPedigreeIdArrayName = nullptr;
  this->SetVertexPedigreeIdArrayName("vertex id");
  this->EdgePedigreeIdArrayName = nullptr;
  this->SetEdgePedigreeIdArrayName("edge id");
  this->EdgeWeightArrayName = nullptr;
  this->SetEdgeWeightArrayName("edge weight");
  this->Seed = 1177;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkRandomGraphSource::~svtkRandomGraphSource()
{
  this->SetVertexPedigreeIdArrayName(nullptr);
  this->SetEdgePedigreeIdArrayName(nullptr);
  this->SetEdgeWeightArrayName(nullptr);
}

// ----------------------------------------------------------------------

void svtkRandomGraphSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfVertices: " << this->NumberOfVertices << endl;
  os << indent << "NumberOfEdges: " << this->NumberOfEdges << endl;
  os << indent << "EdgeProbability: " << this->EdgeProbability << endl;
  os << indent << "IncludeEdgeWeights: " << this->IncludeEdgeWeights << endl;
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "UseEdgeProbability: " << this->UseEdgeProbability << endl;
  os << indent << "StartWithTree: " << this->StartWithTree << endl;
  os << indent << "AllowSelfLoops: " << this->AllowSelfLoops << endl;
  os << indent << "AllowParallelEdges: " << this->AllowParallelEdges << endl;
  os << indent << "GeneratePedigreeIds: " << this->GeneratePedigreeIds << endl;
  os << indent << "VertexPedigreeIdArrayName: "
     << (this->VertexPedigreeIdArrayName ? this->VertexPedigreeIdArrayName : "(null)") << endl;
  os << indent << "EdgePedigreeIdArrayName: "
     << (this->EdgePedigreeIdArrayName ? this->EdgePedigreeIdArrayName : "(null)") << endl;
  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(null)") << endl;
  os << indent << "Seed: " << this->Seed << endl;
}

// ----------------------------------------------------------------------

int svtkRandomGraphSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Seed the random number generator so we can produce repeatable results
  svtkMath::RandomSeed(this->Seed);

  // Create a mutable graph of the appropriate type.
  svtkSmartPointer<svtkMutableDirectedGraph> dirBuilder =
    svtkSmartPointer<svtkMutableDirectedGraph>::New();
  svtkSmartPointer<svtkMutableUndirectedGraph> undirBuilder =
    svtkSmartPointer<svtkMutableUndirectedGraph>::New();

  for (svtkIdType i = 0; i < this->NumberOfVertices; ++i)
  {
    if (this->Directed)
    {
      dirBuilder->AddVertex();
    }
    else
    {
      undirBuilder->AddVertex();
    }
  }

  if (this->StartWithTree)
  {
    for (svtkIdType i = 1; i < this->NumberOfVertices; i++)
    {
      // Pick a random vertex in [0, i-1].
      int j = static_cast<svtkIdType>(svtkMath::Random(0, i));
      if (this->Directed)
      {
        dirBuilder->AddEdge(j, i);
      }
      else
      {
        undirBuilder->AddEdge(j, i);
      }
    }
  }

  if (this->UseEdgeProbability)
  {
    for (svtkIdType i = 0; i < this->NumberOfVertices; i++)
    {
      svtkIdType begin = this->Directed ? 0 : i + 1;
      for (svtkIdType j = begin; j < this->NumberOfVertices; j++)
      {
        double r = svtkMath::Random();
        if (r < this->EdgeProbability)
        {
          if (this->Directed)
          {
            dirBuilder->AddEdge(i, j);
          }
          else
          {
            undirBuilder->AddEdge(i, j);
          }
        }
      }
    }
  }
  else
  {
    // Don't duplicate edges.
    std::set<std::pair<svtkIdType, svtkIdType> > existingEdges;

    svtkIdType MaxEdges;
    if (this->AllowParallelEdges)
    {
      MaxEdges = this->NumberOfEdges;
    }
    else if (this->AllowSelfLoops)
    {
      MaxEdges = this->NumberOfVertices * this->NumberOfVertices;
    }
    else
    {
      MaxEdges = (this->NumberOfVertices * (this->NumberOfVertices - 1)) / 2;
    }

    if (this->NumberOfEdges > MaxEdges)
    {
      this->NumberOfEdges = MaxEdges;
    }

    for (svtkIdType i = 0; i < this->NumberOfEdges; i++)
    {
      bool newEdgeFound = false;
      while (!newEdgeFound)
      {
        svtkIdType s = static_cast<svtkIdType>(svtkMath::Random(0, this->NumberOfVertices));
        svtkIdType t = static_cast<svtkIdType>(svtkMath::Random(0, this->NumberOfVertices));
        if (s == t && (!this->AllowSelfLoops))
        {
          continue;
        }

        if (!this->Directed)
        {
          svtkIdType tmp;
          if (s > t)
          {
            tmp = t;
            t = s;
            s = tmp;
          }
        }

        std::pair<svtkIdType, svtkIdType> newEdge(s, t);

        if (this->AllowParallelEdges || existingEdges.find(newEdge) == existingEdges.end())
        {
          svtkDebugMacro(<< "Adding edge " << s << " to " << t);
          if (this->Directed)
          {
            dirBuilder->AddEdge(s, t);
          }
          else
          {
            undirBuilder->AddEdge(s, t);
          }
          existingEdges.insert(newEdge);
          newEdgeFound = true;
        }
      }
    }
  }

  // Copy the structure into the output.
  svtkGraph* output = svtkGraph::GetData(outputVector);
  if (this->Directed)
  {
    if (!output->CheckedShallowCopy(dirBuilder))
    {
      svtkErrorMacro(<< "Invalid structure.");
      return 0;
    }
  }
  else
  {
    if (!output->CheckedShallowCopy(undirBuilder))
    {
      svtkErrorMacro(<< "Invalid structure.");
      return 0;
    }
  }

  if (this->IncludeEdgeWeights)
  {
    if (!this->EdgeWeightArrayName)
    {
      svtkErrorMacro("When generating edge weights, "
        << "edge weights array name must be defined.");
      return 0;
    }
    svtkFloatArray* weights = svtkFloatArray::New();
    weights->SetName(this->EdgeWeightArrayName);
    for (svtkIdType i = 0; i < output->GetNumberOfEdges(); ++i)
    {
      weights->InsertNextValue(svtkMath::Random());
    }
    output->GetEdgeData()->AddArray(weights);
    weights->Delete();
  }

  if (this->GeneratePedigreeIds)
  {
    if (!this->VertexPedigreeIdArrayName || !this->EdgePedigreeIdArrayName)
    {
      svtkErrorMacro("When generating pedigree ids, "
        << "vertex and edge pedigree id array names must be defined.");
      return 0;
    }
    svtkIdType numVert = output->GetNumberOfVertices();
    svtkSmartPointer<svtkIdTypeArray> vertIds = svtkSmartPointer<svtkIdTypeArray>::New();
    vertIds->SetName(this->VertexPedigreeIdArrayName);
    vertIds->SetNumberOfTuples(numVert);
    for (svtkIdType i = 0; i < numVert; ++i)
    {
      vertIds->SetValue(i, i);
    }
    output->GetVertexData()->SetPedigreeIds(vertIds);

    svtkIdType numEdge = output->GetNumberOfEdges();
    svtkSmartPointer<svtkIdTypeArray> edgeIds = svtkSmartPointer<svtkIdTypeArray>::New();
    edgeIds->SetName(this->EdgePedigreeIdArrayName);
    edgeIds->SetNumberOfTuples(numEdge);
    for (svtkIdType i = 0; i < numEdge; ++i)
    {
      edgeIds->SetValue(i, i);
    }
    output->GetEdgeData()->SetPedigreeIds(edgeIds);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkRandomGraphSource::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  svtkDataObject* current = this->GetExecutive()->GetOutputData(0);
  if (!current || (this->Directed && !svtkDirectedGraph::SafeDownCast(current)) ||
    (!this->Directed && svtkDirectedGraph::SafeDownCast(current)))
  {
    svtkGraph* output = nullptr;
    if (this->Directed)
    {
      output = svtkDirectedGraph::New();
    }
    else
    {
      output = svtkUndirectedGraph::New();
    }
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
  }

  return 1;
}
