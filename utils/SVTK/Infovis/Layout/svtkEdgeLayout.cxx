/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkEdgeLayout.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkEdgeLayoutStrategy.h"
#include "svtkEventForwarderCommand.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"

svtkStandardNewMacro(svtkEdgeLayout);

// ----------------------------------------------------------------------

svtkEdgeLayout::svtkEdgeLayout()
{
  this->LayoutStrategy = nullptr;
  this->InternalGraph = nullptr;

  this->ObserverTag = 0;
  this->EventForwarder = svtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
}

// ----------------------------------------------------------------------

svtkEdgeLayout::~svtkEdgeLayout()
{
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->Delete();
  }
  if (this->InternalGraph)
  {
    this->InternalGraph->Delete();
  }
  this->EventForwarder->Delete();
}

// ----------------------------------------------------------------------

void svtkEdgeLayout::SetLayoutStrategy(svtkEdgeLayoutStrategy* strategy)
{
  // This method is a cut and paste of svtkCxxSetObjectMacro
  // except for the call to SetEdge in the middle :)
  if (strategy != this->LayoutStrategy)
  {
    svtkEdgeLayoutStrategy* tmp = this->LayoutStrategy;
    this->LayoutStrategy = strategy;
    if (this->LayoutStrategy != nullptr)
    {
      this->LayoutStrategy->Register(this);
      this->ObserverTag =
        this->LayoutStrategy->AddObserver(svtkCommand::ProgressEvent, this->EventForwarder);
      if (this->InternalGraph)
      {
        // Set the graph in the layout strategy
        this->LayoutStrategy->SetGraph(this->InternalGraph);
      }
    }
    if (tmp != nullptr)
    {
      tmp->RemoveObserver(this->ObserverTag);
      tmp->UnRegister(this);
    }
    this->Modified();
  }
}

// ----------------------------------------------------------------------

svtkMTimeType svtkEdgeLayout::GetMTime()
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

int svtkEdgeLayout::RequestData(svtkInformation* svtkNotUsed(request),
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

  if (this->InternalGraph)
  {
    this->InternalGraph->Delete();
  }

  this->InternalGraph = input->NewInstance();
  // The strategy object is going to modify the Points member so
  // we'll replace that with a deep copy.  For everything else a
  // shallow copy is sufficient.
  this->InternalGraph->ShallowCopy(input);

  // Copy the edge layout points.
  this->InternalGraph->DeepCopyEdgePoints(input);

  // Give the layout strategy a pointer to the input.  We set it to
  // nullptr first to force the layout algorithm to re-initialize
  // itself.  This is necessary in case the input is the same data
  // object with a newer mtime.
  this->LayoutStrategy->SetGraph(nullptr);
  this->LayoutStrategy->SetGraph(this->InternalGraph);

  // No matter whether the input is new or not, the layout strategy
  // needs to do its thing.  It modifies its input
  // (this->InternalGraph) so we can just use that as the output.
  this->LayoutStrategy->Layout();
  output->ShallowCopy(this->InternalGraph);

  return 1;
}

// ----------------------------------------------------------------------

void svtkEdgeLayout::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
}
