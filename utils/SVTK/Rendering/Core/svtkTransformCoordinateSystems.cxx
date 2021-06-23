/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformCoordinateSystems.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransformCoordinateSystems.h"

#include "svtkCoordinate.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkTransformCoordinateSystems);

//------------------------------------------------------------------------
svtkTransformCoordinateSystems::svtkTransformCoordinateSystems()
{
  this->TransformCoordinate = svtkCoordinate::New();
  this->TransformCoordinate->SetCoordinateSystemToWorld();
  this->InputCoordinateSystem = SVTK_WORLD;
  this->OutputCoordinateSystem = SVTK_DISPLAY;
  this->Viewport = nullptr;
}

//------------------------------------------------------------------------
svtkTransformCoordinateSystems::~svtkTransformCoordinateSystems()
{
  this->TransformCoordinate->Delete();
}

// ----------------------------------------------------------------------------
// Set the viewport. This is a raw pointer, not a weak pointer or a reference
// counted object to avoid cycle reference loop between rendering classes
// and filter classes.
void svtkTransformCoordinateSystems::SetViewport(svtkViewport* viewport)
{
  if (this->Viewport != viewport)
  {
    this->Viewport = viewport;
    this->Modified();
  }
}

//------------------------------------------------------------------------
int svtkTransformCoordinateSystems::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkIdType numPts;

  svtkDebugMacro(<< "Executing transform coordinates filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);
  output->CopyAttributes(input);

  // Check input
  //
  inPts = input->GetPoints();

  if (!inPts)
  {
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();

  newPts = svtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  this->UpdateProgress(.2);

  // Configure the input
  this->TransformCoordinate->SetViewport(this->Viewport);
  switch (this->InputCoordinateSystem)
  {
    case SVTK_DISPLAY:
      this->TransformCoordinate->SetCoordinateSystemToDisplay();
      break;
    case SVTK_VIEWPORT:
      this->TransformCoordinate->SetCoordinateSystemToViewport();
      break;
    case SVTK_WORLD:
      this->TransformCoordinate->SetCoordinateSystemToWorld();
      break;
  }

  // Loop over all points, updating position
  svtkIdType ptId;
  double* itmp;
  if (this->OutputCoordinateSystem == SVTK_DISPLAY)
  {
    for (ptId = 0; ptId < numPts; ptId++)
    {
      this->TransformCoordinate->SetValue(inPts->GetPoint(ptId));
      itmp = this->TransformCoordinate->GetComputedDoubleDisplayValue(this->Viewport);
      newPts->SetPoint(ptId, itmp[0], itmp[1], 0.0);
    }
  }
  else if (this->OutputCoordinateSystem == SVTK_VIEWPORT)
  {
    for (ptId = 0; ptId < numPts; ptId++)
    {
      this->TransformCoordinate->SetValue(inPts->GetPoint(ptId));
      itmp = this->TransformCoordinate->GetComputedDoubleViewportValue(this->Viewport);
      newPts->SetPoint(ptId, itmp[0], itmp[1], 0.0);
    }
  }
  else if (this->OutputCoordinateSystem == SVTK_WORLD)
  {
    for (ptId = 0; ptId < numPts; ptId++)
    {
      this->TransformCoordinate->SetValue(inPts->GetPoint(ptId));
      itmp = this->TransformCoordinate->GetComputedWorldValue(this->Viewport);
      newPts->SetPoint(ptId, itmp[0], itmp[1], itmp[2]);
    }
  }
  this->UpdateProgress(.9);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

//------------------------------------------------------------------------
svtkMTimeType svtkTransformCoordinateSystems::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType viewMTime;

  if (this->Viewport)
  {
    viewMTime = this->Viewport->GetMTime();
    mTime = (viewMTime > mTime ? viewMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------
void svtkTransformCoordinateSystems::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input Coordinate System: ";
  if (this->InputCoordinateSystem == SVTK_DISPLAY)
  {
    os << " DISPLAY\n";
  }
  else if (this->InputCoordinateSystem == SVTK_WORLD)
  {
    os << " WORLD\n";
  }
  else // if ( this->InputCoordinateSystem == SVTK_VIEWPORT )
  {
    os << " VIEWPORT\n";
  }

  os << indent << "Output Coordinate System: ";
  if (this->OutputCoordinateSystem == SVTK_DISPLAY)
  {
    os << " DISPLAY\n";
  }
  else if (this->OutputCoordinateSystem == SVTK_WORLD)
  {
    os << " WORLD\n";
  }
  else // if ( this->OutputCoordinateSystem == SVTK_VIEWPORT )
  {
    os << " VIEWPORT\n";
  }

  os << indent << "Viewport: ";
  if (this->Viewport)
  {
    os << this->Viewport << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}
