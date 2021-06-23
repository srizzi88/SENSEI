/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPWarpVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSMPWarpVector.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkSMPWarpVector);

//----------------------------------------------------------------------------
svtkSMPWarpVector::svtkSMPWarpVector()
{
  this->ScaleFactor = 1.0;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);

  SVTK_LEGACY_BODY(svtkSMPWarpVector::svtkSMPWarpVector, "SVTK 8.1");
}

//----------------------------------------------------------------------------
svtkSMPWarpVector::~svtkSMPWarpVector() = default;

//----------------------------------------------------------------------------
template <class PointArrayType, class VecArrayType>
class svtkSMPWarpVectorOp
{
  using ScaleT = svtk::GetAPIType<PointArrayType>;

public:
  PointArrayType* InPoints;
  PointArrayType* OutPoints;
  VecArrayType* InVector;
  double scaleFactor;

  void operator()(svtkIdType begin, svtkIdType end) const
  {
    auto inPts = svtk::DataArrayTupleRange<3>(this->InPoints, begin, end);
    auto inVec = svtk::DataArrayTupleRange<3>(this->InVector, begin, end);
    auto outPts = svtk::DataArrayTupleRange<3>(this->OutPoints, begin, end);
    const ScaleT sf = static_cast<ScaleT>(this->scaleFactor);

    const svtkIdType size = end - begin;
    for (svtkIdType index = 0; index < size; index++)
    {
      outPts[index][0] = inPts[index][0] + sf * static_cast<ScaleT>(inVec[index][0]);
      outPts[index][1] = inPts[index][1] + sf * static_cast<ScaleT>(inVec[index][1]);
      outPts[index][2] = inPts[index][2] + sf * static_cast<ScaleT>(inVec[index][2]);
    }
  }
};

//----------------------------------------------------------------------------
struct svtkSMPWarpVectorExecute
{
  template <class T1, class T2>
  void operator()(
    T1* inPtsArray, T2* inVecArray, svtkDataArray* outDataArray, double scaleFactor) const
  {

    T1* outArray = svtkArrayDownCast<T1>(outDataArray); // known to be same as
                                                       // input
    svtkSMPWarpVectorOp<T1, T2> op{ inPtsArray, outArray, inVecArray, scaleFactor };
    svtkSMPTools::For(0, inPtsArray->GetNumberOfTuples(), op);
  }
};

//----------------------------------------------------------------------------
int svtkSMPWarpVector::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    // Let the superclass handle svtkImageData and svtkRectilinearGrid
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }
  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

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

  svtkDataArray* inpts = input->GetPoints()->GetData();
  svtkDataArray* outpts = output->GetPoints()->GetData();

  if (!svtkArrayDispatch::Dispatch2::Execute(
        inpts, vectors, svtkSMPWarpVectorExecute{}, outpts, this->ScaleFactor))
  {
    svtkSMPWarpVectorExecute{}(inpts, vectors, outpts, this->ScaleFactor);
  }

  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//----------------------------------------------------------------------------
void svtkSMPWarpVector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
