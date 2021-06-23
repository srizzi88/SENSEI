/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageNoiseSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageNoiseSource.h"

#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageNoiseSource);

//----------------------------------------------------------------------------
svtkImageNoiseSource::svtkImageNoiseSource()
{
  this->Minimum = 0.0;
  this->Maximum = 10.0;
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
void svtkImageNoiseSource::SetWholeExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax)
{
  int modified = 0;

  if (this->WholeExtent[0] != xMin)
  {
    modified = 1;
    this->WholeExtent[0] = xMin;
  }
  if (this->WholeExtent[1] != xMax)
  {
    modified = 1;
    this->WholeExtent[1] = xMax;
  }
  if (this->WholeExtent[2] != yMin)
  {
    modified = 1;
    this->WholeExtent[2] = yMin;
  }
  if (this->WholeExtent[3] != yMax)
  {
    modified = 1;
    this->WholeExtent[3] = yMax;
  }
  if (this->WholeExtent[4] != zMin)
  {
    modified = 1;
    this->WholeExtent[4] = zMin;
  }
  if (this->WholeExtent[5] != zMax)
  {
    modified = 1;
    this->WholeExtent[5] = zMax;
  }
  if (modified)
  {
    this->Modified();
  }
}
//----------------------------------------------------------------------------
int svtkImageNoiseSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkDataObject::SPACING(), 1.0, 1.0, 1.0);
  outInfo->Set(svtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_DOUBLE, 1);
  return 1;
}

void svtkImageNoiseSource::ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo)
{
  svtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (data->GetScalarType() != SVTK_DOUBLE)
  {
    svtkErrorMacro("Execute: This source only outputs doubles");
  }

  svtkImageProgressIterator<double> outIt(data, data->GetExtent(), this, 0);

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    double* outSI = outIt.BeginSpan();
    double* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // now process the components
      *outSI = this->Minimum + (this->Maximum - this->Minimum) * svtkMath::Random();
      outSI++;
    }
    outIt.NextSpan();
  }
}

void svtkImageNoiseSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Minimum: " << this->Minimum << "\n";
  os << indent << "Maximum: " << this->Maximum << "\n";
}
