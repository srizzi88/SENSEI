/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphLayout.cxx

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

#include "svtkGraphLayout.h"

#include "svtkAbstractTransform.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkEventForwarderCommand.h"
#include "svtkFloatArray.h"
#include "svtkGraphLayoutStrategy.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkGraphLayout);
svtkCxxSetObjectMacro(svtkGraphLayout, Transform, svtkAbstractTransform);

// ----------------------------------------------------------------------

svtkGraphLayout::svtkGraphLayout()
{
  this->LayoutStrategy = nullptr;
  this->StrategyChanged = false;
  this->LastInput = nullptr;
  this->LastInputMTime = 0;
  this->InternalGraph = nullptr;
  this->ZRange = 0.0;
  this->Transform = nullptr;
  this->UseTransform = false;

  this->EventForwarder = svtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
}

// ----------------------------------------------------------------------

svtkGraphLayout::~svtkGraphLayout()
{
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->RemoveObserver(this->EventForwarder);
    this->LayoutStrategy->Delete();
  }
  if (this->InternalGraph)
  {
    this->InternalGraph->Delete();
  }
  if (this->Transform)
  {
    this->Transform->Delete();
  }
  this->EventForwarder->Delete();
}

// ----------------------------------------------------------------------

void svtkGraphLayout::SetLayoutStrategy(svtkGraphLayoutStrategy* strategy)
{
  // This method is a cut and paste of svtkCxxSetObjectMacro
  // except for the call to SetGraph in the middle :)
  if (strategy != this->LayoutStrategy)
  {
    svtkGraphLayoutStrategy* tmp = this->LayoutStrategy;
    if (tmp)
    {
      tmp->RemoveObserver(this->EventForwarder);
    }
    this->LayoutStrategy = strategy;
    if (this->LayoutStrategy != nullptr)
    {
      this->StrategyChanged = true;
      this->LayoutStrategy->Register(this);
      this->LayoutStrategy->AddObserver(svtkCommand::ProgressEvent, this->EventForwarder);
      if (this->InternalGraph)
      {
        // Set the graph in the layout strategy
        this->LayoutStrategy->SetGraph(this->InternalGraph);
      }
    }
    if (tmp != nullptr)
    {
      tmp->UnRegister(this);
    }
    this->Modified();
  }
}

// ----------------------------------------------------------------------

svtkMTimeType svtkGraphLayout::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->LayoutStrategy != nullptr)
  {
    time = this->LayoutStrategy->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

// ----------------------------------------------------------------------

int svtkGraphLayout::IsLayoutComplete()
{
  if (this->LayoutStrategy)
  {
    return this->LayoutStrategy->IsLayoutComplete();
  }

  // This is an error condition
  svtkErrorMacro("IsLayoutComplete called with layout strategy==nullptr");
  return 0;
}

// ----------------------------------------------------------------------

int svtkGraphLayout::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->LayoutStrategy == nullptr)
  {
    svtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
  }

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Is this a completely new input?  Is it the same input as the last
  // time the filter ran but with a new MTime?  If either of those is
  // true, make a copy and give it to the strategy object anew.
  if (this->StrategyChanged || input != this->LastInput || input->GetMTime() > this->LastInputMTime)
  {
    if (this->StrategyChanged)
    {
      svtkDebugMacro(<< "Strategy changed so reading in input again.");
      this->StrategyChanged = false;
    }
    else if (input != this->LastInput)
    {
      svtkDebugMacro(<< "Filter running with different input.  Resetting in strategy.");
    }
    else
    {
      svtkDebugMacro(<< "Input modified since last run.  Resetting in strategy.");
    }

    if (this->InternalGraph)
    {
      this->InternalGraph->Delete();
    }

    this->InternalGraph = input->NewInstance();
    // The strategy object is going to modify the Points member so
    // we'll replace that with a deep copy.  For everything else a
    // shallow copy is sufficient.
    this->InternalGraph->ShallowCopy(input);

    // The copy of the points will be to a float type
    svtkPoints* newPoints = svtkPoints::New(SVTK_FLOAT);
    newPoints->DeepCopy(input->GetPoints());
    this->InternalGraph->SetPoints(newPoints);
    newPoints->Delete();

    // Save information about the input so that we can detect when
    // it's changed on future runs.  According to the SVTK pipeline
    // design, this is a bad thing.  In this case there's no
    // particularly graceful way around it since the pipeline was not
    // designed to support incremental execution.
    this->LastInput = input;
    this->LastInputMTime = input->GetMTime();

    // Give the layout strategy a pointer to the input.  We set it to
    // nullptr first to force the layout algorithm to re-initialize
    // itself.  This is necessary in case the input is the same data
    // object with a newer mtime.
    this->LayoutStrategy->SetGraph(nullptr);
    this->LayoutStrategy->SetGraph(this->InternalGraph);
  } // Done handling a new or changed filter input.

  // No matter whether the input is new or not, the layout strategy
  // needs to do its thing.  It modifies its input
  // (this->InternalGraph) so we can just use that as the output.
  this->LayoutStrategy->Layout();
  output->ShallowCopy(this->InternalGraph);

  // Perturb points so they do not all have the same z value.
  if (this->ZRange != 0.0)
  {
    svtkIdType numVert = output->GetNumberOfVertices();
    double x[3];
    bool onPlane = true;
    for (svtkIdType i = 0; i < numVert; ++i)
    {
      output->GetPoint(i, x);
      if (x[2] != 0.0)
      {
        onPlane = false;
        break;
      }
    }
    if (onPlane)
    {
      svtkPoints* pts = svtkPoints::New();
      pts->SetNumberOfPoints(numVert);
      for (svtkIdType i = 0; i < numVert; ++i)
      {
        output->GetPoint(i, x);
        x[2] = this->ZRange * static_cast<double>(i) / numVert;
        pts->SetPoint(i, x);
      }
      output->SetPoints(pts);
      pts->Delete();
    }
  }

  if (this->UseTransform && this->Transform)
  {
    svtkIdType numVert = output->GetNumberOfVertices();
    double x[3];
    double y[3];
    svtkPoints* pts = svtkPoints::New();
    pts->SetNumberOfPoints(numVert);
    for (svtkIdType i = 0; i < numVert; ++i)
    {
      output->GetPoint(i, x);
      this->Transform->TransformPoint(x, y);
      pts->SetPoint(i, y);
    }
    output->SetPoints(pts);
    pts->Delete();
  }

  return 1;
}

// ----------------------------------------------------------------------

void svtkGraphLayout::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StrategyChanged: " << (this->StrategyChanged ? "True" : "False") << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "InternalGraph: " << (this->InternalGraph ? "" : "(none)") << endl;
  if (this->InternalGraph)
  {
    this->InternalGraph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "ZRange: " << this->ZRange << endl;
  os << indent << "Transform: " << (this->Transform ? "" : "(none)") << endl;
  if (this->Transform)
  {
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "UseTransform: " << (this->UseTransform ? "True" : "False") << endl;
}
