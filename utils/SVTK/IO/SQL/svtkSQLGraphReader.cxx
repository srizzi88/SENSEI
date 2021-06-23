/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLGraphReader.cxx

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

#include "svtkSQLGraphReader.h"

#include "svtkAssignCoordinates.h"
#include "svtkDirectedGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRowQueryToTable.h"
#include "svtkSQLQuery.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"
#include "svtkTableToGraph.h"
#include "svtkUndirectedGraph.h"

svtkStandardNewMacro(svtkSQLGraphReader);

svtkSQLGraphReader::svtkSQLGraphReader()
{
  this->Directed = 0;
  this->CollapseEdges = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->VertexQuery = nullptr;
  this->EdgeQuery = nullptr;
  this->SourceField = 0;
  this->TargetField = 0;
  this->VertexIdField = 0;
  this->XField = 0;
  this->YField = 0;
  this->ZField = 0;
}

svtkSQLGraphReader::~svtkSQLGraphReader()
{
  if (this->VertexQuery != nullptr)
  {
    this->VertexQuery->Delete();
  }
  if (this->EdgeQuery != nullptr)
  {
    this->EdgeQuery->Delete();
  }
  this->SetSourceField(0);
  this->SetTargetField(0);
  this->SetVertexIdField(0);
  this->SetXField(0);
  this->SetYField(0);
  this->SetZField(0);
}

void svtkSQLGraphReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "CollapseEdges: " << this->CollapseEdges << endl;
  os << indent << "XField: " << (this->XField ? this->XField : "(null)") << endl;
  os << indent << "YField: " << (this->YField ? this->YField : "(null)") << endl;
  os << indent << "ZField: " << (this->ZField ? this->ZField : "(null)") << endl;
  os << indent << "VertexIdField: " << (this->VertexIdField ? this->VertexIdField : "(null)")
     << endl;
  os << indent << "SourceField: " << (this->SourceField ? this->SourceField : "(null)") << endl;
  os << indent << "TargetField: " << (this->TargetField ? this->TargetField : "(null)") << endl;
  os << indent << "EdgeQuery: " << (this->EdgeQuery ? "" : "(null)") << endl;
  if (this->EdgeQuery)
  {
    this->EdgeQuery->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "VertexQuery: " << (this->VertexQuery ? "" : "(null)") << endl;
  if (this->VertexQuery)
  {
    this->VertexQuery->PrintSelf(os, indent.GetNextIndent());
  }
}

svtkCxxSetObjectMacro(svtkSQLGraphReader, VertexQuery, svtkSQLQuery);
svtkCxxSetObjectMacro(svtkSQLGraphReader, EdgeQuery, svtkSQLQuery);

int svtkSQLGraphReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Check for valid inputs
  if (this->EdgeQuery == nullptr)
  {
    svtkErrorMacro("The EdgeQuery must be defined");
    return 0;
  }
  if (this->SourceField == nullptr)
  {
    svtkErrorMacro("The SourceField must be defined");
    return 0;
  }
  if (this->TargetField == nullptr)
  {
    svtkErrorMacro("The TargetField must be defined");
    return 0;
  }
  if (this->VertexQuery != nullptr)
  {
    if (this->VertexIdField == nullptr)
    {
      svtkErrorMacro("The VertexIdField must be defined when using a VertexQuery");
      return 0;
    }
    if (this->XField != nullptr && this->YField == nullptr)
    {
      svtkErrorMacro("The YField must be defined if the XField is defined");
      return 0;
    }
  }

  svtkGraph* output = svtkGraph::GetData(outputVector);

  svtkTableToGraph* filter = svtkTableToGraph::New();
  filter->SetDirected(this->Directed);

  // Set up the internal filter to use the edge table
  svtkSmartPointer<svtkRowQueryToTable> edgeReader = svtkSmartPointer<svtkRowQueryToTable>::New();
  edgeReader->SetQuery(this->EdgeQuery);
  edgeReader->Update();

  const char* domain = "default";
  if (this->VertexIdField)
  {
    domain = this->VertexIdField;
  }

  filter->SetInputConnection(edgeReader->GetOutputPort());
  filter->AddLinkVertex(this->SourceField, domain);
  filter->AddLinkVertex(this->TargetField, domain);
  filter->AddLinkEdge(this->SourceField, this->TargetField);

  svtkSmartPointer<svtkAssignCoordinates> assign = svtkSmartPointer<svtkAssignCoordinates>::New();
  assign->SetInputConnection(filter->GetOutputPort());

  // Set up the internal filter to use the vertex table
  if (this->VertexQuery != nullptr)
  {
    svtkSmartPointer<svtkRowQueryToTable> vertexReader = svtkSmartPointer<svtkRowQueryToTable>::New();
    vertexReader->SetQuery(this->VertexQuery);
    vertexReader->Update();
    filter->SetInputConnection(1, vertexReader->GetOutputPort());
    if (this->XField != nullptr)
    {
      assign->SetXCoordArrayName(this->XField);
      assign->SetYCoordArrayName(this->YField);
      if (this->ZField != nullptr)
      {
        assign->SetZCoordArrayName(this->ZField);
      }
    }
  }

  // Get the internal filter output and assign it to the output
  if (this->XField != nullptr)
  {
    assign->Update();
    svtkGraph* assignOutput = svtkGraph::SafeDownCast(assign->GetOutput());
    output->ShallowCopy(assignOutput);
  }
  else
  {
    filter->Update();
    svtkGraph* filterOutput = filter->GetOutput();
    output->ShallowCopy(filterOutput);
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }
  output->GetInformation()->Set(svtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(svtkDataObject::DATA_PIECE_NUMBER(), piece);

  // Clean up
  filter->Delete();

  return 1;
}

int svtkSQLGraphReader::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  svtkDataObject* current = this->GetExecutive()->GetOutputData(0);
  if (!current || (this->Directed && !svtkDirectedGraph::SafeDownCast(current)) ||
    (!this->Directed && svtkDirectedGraph::SafeDownCast(current)))
  {
    svtkGraph* output = 0;
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
