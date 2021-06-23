/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWarpVector.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkStructuredGrid.h"

#include "svtkNew.h"
#include "svtkSmartPointer.h"

#include <cstdlib>

svtkStandardNewMacro(svtkWarpVector);

//----------------------------------------------------------------------------
svtkWarpVector::svtkWarpVector()
{
  this->ScaleFactor = 1.0;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
svtkWarpVector::~svtkWarpVector() = default;

//----------------------------------------------------------------------------
int svtkWarpVector::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkWarpVector::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
  svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    svtkStructuredGrid* output = svtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      svtkNew<svtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//----------------------------------------------------------------------------
namespace
{
// Used by the WarpVectorDispatch1Vector worker, defined below:
template <typename VectorArrayT>
struct WarpVectorDispatch2Points
{
  svtkWarpVector* Self;
  VectorArrayT* Vectors;

  WarpVectorDispatch2Points(svtkWarpVector* self, VectorArrayT* vectors)
    : Self(self)
    , Vectors(vectors)
  {
  }

  template <typename InPointArrayT, typename OutPointArrayT>
  void operator()(InPointArrayT* inPtArray, OutPointArrayT* outPtArray)
  {
    typedef typename OutPointArrayT::ValueType PointValueT;
    const svtkIdType numTuples = inPtArray->GetNumberOfTuples();
    const double scaleFactor = this->Self->GetScaleFactor();

    assert(this->Vectors->GetNumberOfComponents() == 3);
    assert(inPtArray->GetNumberOfComponents() == 3);
    assert(outPtArray->GetNumberOfComponents() == 3);

    for (svtkIdType t = 0; t < numTuples; ++t)
    {
      if (!(t & 0xfff))
      {
        this->Self->UpdateProgress(t / static_cast<double>(numTuples));
        if (this->Self->GetAbortExecute())
        {
          return;
        }
      }

      for (int c = 0; c < 3; ++c)
      {
        PointValueT val =
          inPtArray->GetTypedComponent(t, c) + scaleFactor * this->Vectors->GetTypedComponent(t, c);
        outPtArray->SetTypedComponent(t, c, val);
      }
    }
  }
};

// Dispatch just the vector array first, we can cut out some generated code
// since the point arrays will have the same type.
struct WarpVectorDispatch1Vector
{
  svtkWarpVector* Self;
  svtkDataArray* InPoints;
  svtkDataArray* OutPoints;

  WarpVectorDispatch1Vector(svtkWarpVector* self, svtkDataArray* inPoints, svtkDataArray* outPoints)
    : Self(self)
    , InPoints(inPoints)
    , OutPoints(outPoints)
  {
  }

  template <typename VectorArrayT>
  void operator()(VectorArrayT* vectors)
  {
    WarpVectorDispatch2Points<VectorArrayT> worker(this->Self, vectors);
    if (!svtkArrayDispatch::Dispatch2SameValueType::Execute(this->InPoints, this->OutPoints, worker))
    {
      svtkGenericWarningMacro("Error dispatching point arrays.");
    }
  }
};
} // end anon namespace

//----------------------------------------------------------------------------
int svtkWarpVector::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkPointSet> input = svtkPointSet::GetData(inputVector[0]);
  svtkPointSet* output = svtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      svtkNew<svtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      svtkNew<svtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    svtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  svtkPoints* points;
  svtkIdType numPts;

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (input == nullptr || input->GetPoints() == nullptr)
  {
    return 1;
  }
  numPts = input->GetPoints()->GetNumberOfPoints();

  svtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);

  if (!vectors || !numPts)
  {
    svtkDebugMacro(<< "No input data");
    return 1;
  }

  // SETUP AND ALLOCATE THE OUTPUT
  numPts = input->GetNumberOfPoints();
  points = input->GetPoints()->NewInstance();
  points->SetDataType(input->GetPoints()->GetDataType());
  points->Allocate(numPts);
  points->SetNumberOfPoints(numPts);
  output->SetPoints(points);
  points->Delete();

  // call templated function.
  // We use two dispatches since we need to dispatch 3 arrays and two share a
  // value type. Implementing a second type-restricted dispatch reduces
  // the amount of generated templated code.
  WarpVectorDispatch1Vector worker(
    this, input->GetPoints()->GetData(), output->GetPoints()->GetData());
  if (!svtkArrayDispatch::Dispatch::Execute(vectors, worker))
  {
    svtkWarningMacro("Dispatch failed for vector array.");
  }

  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//----------------------------------------------------------------------------
void svtkWarpVector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
