/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageToPoints.h"

#include <svtkImageData.h>
#include <svtkImagePointIterator.h>
#include <svtkImageStencilData.h>
#include <svtkInformation.h>
#include <svtkInformationVector.h>
#include <svtkMath.h>
#include <svtkObjectFactory.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkSmartPointer.h>
#include <svtkStreamingDemandDrivenPipeline.h>

svtkStandardNewMacro(svtkImageToPoints);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageToPoints::svtkImageToPoints()
{
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkImageToPoints::~svtkImageToPoints() = default;

//----------------------------------------------------------------------------
void svtkImageToPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}

//----------------------------------------------------------------------------
void svtkImageToPoints::SetStencilConnection(svtkAlgorithmOutput* stencil)
{
  this->SetInputConnection(1, stencil);
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkImageToPoints::GetStencilConnection()
{
  return this->GetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
void svtkImageToPoints::SetStencilData(svtkImageStencilData* stencil)
{
  this->SetInputData(1, stencil);
}

//----------------------------------------------------------------------------
int svtkImageToPoints::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageStencilData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToPoints::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToPoints::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToPoints::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int inExt[6];
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the input extent
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    svtkInformation* stencilInfo = inputVector[1]->GetInformationObject(0);
    stencilInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
  }

  return 1;
}

namespace
{

//----------------------------------------------------------------------------
svtkIdType svtkImageToPointsCount(
  svtkImageData* inData, svtkImageStencilData* stencil, const int extent[6])
{
  // count the number of points so that we can pre-allocate the space
  svtkIdType count = 0;

  // iterate over all spans for the stencil
  svtkImagePointDataIterator inIter(inData, extent, stencil);
  for (; !inIter.IsAtEnd(); inIter.NextSpan())
  {
    if (inIter.IsInStencil())
    {
      count += inIter.SpanEndId() - inIter.GetId();
    }
  }

  return count;
}

//----------------------------------------------------------------------------
// The execute method is templated over the point type (float or double)
template <class T>
void svtkImageToPointsExecute(svtkImageToPoints* self, svtkImageData* inData, const int extent[6],
  svtkImageStencilData* stencil, T* outPoints, svtkPointData* outPD)
{
  svtkPointData* inPD = inData->GetPointData();
  svtkImagePointIterator inIter(inData, extent, stencil, self, 0);
  svtkIdType outId = 0;

  // iterate over all spans for the stencil
  while (!inIter.IsAtEnd())
  {
    if (inIter.IsInStencil())
    {
      // if span is inside stencil, generate points
      svtkIdType n = inIter.SpanEndId() - inIter.GetId();
      outPD->CopyData(inPD, outId, n, inIter.GetId());
      outId += n;
      for (svtkIdType i = 0; i < n; i++)
      {
        inIter.GetPosition(outPoints);
        outPoints += 3;
        inIter.Next();
      }
    }
    else
    {
      // if span is outside stencil, skip to next span
      inIter.NextSpan();
    }
  }
}

}

//----------------------------------------------------------------------------
int svtkImageToPoints::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input
  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  svtkInformation* stencilInfo = inputVector[1]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

  // use a stencil, if a stencil is connected
  svtkImageStencilData* stencil = nullptr;
  if (stencilInfo)
  {
    stencil = static_cast<svtkImageStencilData*>(stencilInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  // get the requested precision
  int pointsType = SVTK_DOUBLE;
  if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    pointsType = SVTK_FLOAT;
  }

  // get the output data object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkPolyData* outData = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // count the total number of output points
  const int* extent = inData->GetExtent();
  svtkIdType numPoints = svtkImageToPointsCount(inData, stencil, extent);

  // create the points
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetDataType(pointsType);
  points->SetNumberOfPoints(numPoints);
  outData->SetPoints(points);

  // pre-allocate output arrays
  svtkPointData* outPD = outData->GetPointData();
  outPD->CopyAllocate(inData->GetPointData(), numPoints);

  // iterate over the input and create the point data
  void* ptr = points->GetVoidPointer(0);
  if (pointsType == SVTK_FLOAT)
  {
    svtkImageToPointsExecute(this, inData, extent, stencil, static_cast<float*>(ptr), outPD);
  }
  else
  {
    svtkImageToPointsExecute(this, inData, extent, stencil, static_cast<double*>(ptr), outPD);
  }

  return 1;
}
