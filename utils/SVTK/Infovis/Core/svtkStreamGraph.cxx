/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreamGraph.cxx

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

#include "svtkStreamGraph.h"

#include "svtkCommand.h"
#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergeGraphs.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableGraphHelper.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include <map>

svtkStandardNewMacro(svtkStreamGraph);
//---------------------------------------------------------------------------
svtkStreamGraph::svtkStreamGraph()
{
  this->CurrentGraph = svtkMutableGraphHelper::New();
  this->MergeGraphs = svtkMergeGraphs::New();
  this->UseEdgeWindow = false;
  this->EdgeWindowArrayName = nullptr;
  this->SetEdgeWindowArrayName("time");
  this->EdgeWindow = 10000.0;
}

//---------------------------------------------------------------------------
svtkStreamGraph::~svtkStreamGraph()
{
  if (this->CurrentGraph)
  {
    this->CurrentGraph->Delete();
  }
  if (this->MergeGraphs)
  {
    this->MergeGraphs->Delete();
  }
  this->SetEdgeWindowArrayName(nullptr);
}

//---------------------------------------------------------------------------
int svtkStreamGraph::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* input_info = inputVector[0]->GetInformationObject(0);
  svtkGraph* input = svtkGraph::SafeDownCast(input_info->Get(svtkDataObject::DATA_OBJECT()));

  // Copy structure into output graph.
  svtkInformation* outputInfo = outputVector->GetInformationObject(0);
  svtkGraph* output = svtkGraph::SafeDownCast(outputInfo->Get(svtkDataObject::DATA_OBJECT()));

  double progress = 0.1;
  this->InvokeEvent(svtkCommand::ProgressEvent, &progress);

  // First pass: make a copy of the graph and we're done
  if (!this->CurrentGraph->GetGraph())
  {
    if (svtkDirectedGraph::SafeDownCast(input))
    {
      svtkSmartPointer<svtkMutableDirectedGraph> g = svtkSmartPointer<svtkMutableDirectedGraph>::New();
      this->CurrentGraph->SetGraph(g);
    }
    else
    {
      svtkSmartPointer<svtkMutableUndirectedGraph> g =
        svtkSmartPointer<svtkMutableUndirectedGraph>::New();
      this->CurrentGraph->SetGraph(g);
    }
    this->CurrentGraph->GetGraph()->DeepCopy(input);
    if (!output->CheckedShallowCopy(input))
    {
      svtkErrorMacro("Output graph format invalid.");
      return 0;
    }
    return 1;
  }

  progress = 0.2;
  this->InvokeEvent(svtkCommand::ProgressEvent, &progress);

  this->MergeGraphs->SetUseEdgeWindow(this->UseEdgeWindow);
  this->MergeGraphs->SetEdgeWindowArrayName(this->EdgeWindowArrayName);
  this->MergeGraphs->SetEdgeWindow(this->EdgeWindow);

  if (!this->MergeGraphs->ExtendGraph(this->CurrentGraph, input))
  {
    return 0;
  }

  progress = 0.9;
  this->InvokeEvent(svtkCommand::ProgressEvent, &progress);

  output->DeepCopy(this->CurrentGraph->GetGraph());

  return 1;
}

//---------------------------------------------------------------------------
void svtkStreamGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseEdgeWindow: " << this->UseEdgeWindow << endl;
  os << indent << "EdgeWindowArrayName: "
     << (this->EdgeWindowArrayName ? this->EdgeWindowArrayName : "(none)") << endl;
  os << indent << "EdgeWindow: " << this->EdgeWindow << endl;
}
