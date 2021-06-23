/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLinearExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPLinearExtrusionFilter.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPLinearExtrusionFilter);

//----------------------------------------------------------------------------
svtkPLinearExtrusionFilter::svtkPLinearExtrusionFilter()
{
  // Since I do not thing the visual impact of invariance is significant,
  // we will default to not spend the extra effort to get the
  // extra layer of ghost cells.
  this->PieceInvariant = 0;
}

//----------------------------------------------------------------------------
int svtkPLinearExtrusionFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->svtkLinearExtrusionFilter::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  return 1;
}

//--------------------------------------------------------------------------
int svtkPLinearExtrusionFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));

  int ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (this->PieceInvariant)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel + 1);
  }
  else
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkPLinearExtrusionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PieceInvariant: " << this->PieceInvariant << "\n";
}
