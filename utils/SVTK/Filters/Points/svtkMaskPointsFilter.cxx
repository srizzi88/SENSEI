/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskPointsFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMaskPointsFilter.h"

#include "svtkArrayDispatch.h"
#include "svtkDataArrayRange.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkMaskPointsFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm
struct ExtractPoints
{
  template <typename PtArrayT>
  void operator()(PtArrayT* ptArray, const unsigned char* mask, unsigned char emptyValue,
    const int dims[3], const double origin[3], const double spacing[3], svtkIdType* pointMap) const
  {
    const svtkIdType numPts = ptArray->GetNumberOfTuples();

    const double fX = 1.0 / spacing[0];
    const double fY = 1.0 / spacing[1];
    const double fZ = 1.0 / spacing[2];

    const double bX = origin[0] - 0.5 * spacing[0];
    const double bY = origin[1] - 0.5 * spacing[1];
    const double bZ = origin[2] - 0.5 * spacing[2];

    const svtkIdType xD = dims[0];
    const svtkIdType yD = dims[1];
    const svtkIdType zD = dims[2];
    const svtkIdType xyD = dims[0] * dims[1];

    svtkSMPTools::For(0, numPts, [&](svtkIdType ptId, svtkIdType endPtId) {
      const auto pts = svtk::DataArrayTupleRange<3>(ptArray, ptId, endPtId);
      using PtCRefT = typename decltype(pts)::ConstTupleReferenceType;

      svtkIdType* map = pointMap + ptId;

      // MSVC 2015 x64 ICEs when this loop is written using std::transform,
      // so we'll work around that by using a for-range loop:
      for (PtCRefT pt : pts)
      {
        const int i = static_cast<int>(((pt[0] - bX) * fX));
        const int j = static_cast<int>(((pt[1] - bY) * fY));
        const int k = static_cast<int>(((pt[2] - bZ) * fZ));

        // If not inside image then skip
        if (i < 0 || i >= xD || j < 0 || j >= yD || k < 0 || k >= zD)
        {
          *map++ = -1;
        }
        else if (mask[i + j * xD + k * xyD] != emptyValue)
        {
          *map++ = 1;
        }
        else
        {
          *map++ = -1;
        }
      }
    });
  }
}; // ExtractPoints

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkMaskPointsFilter::svtkMaskPointsFilter()
{
  this->SetNumberOfInputPorts(2);

  this->EmptyValue = 0;
  this->Mask = nullptr;
}

//----------------------------------------------------------------------------
svtkMaskPointsFilter::~svtkMaskPointsFilter() = default;

//----------------------------------------------------------------------------
int svtkMaskPointsFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkMaskPointsFilter::SetMaskConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkMaskPointsFilter::SetMaskData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkMaskPointsFilter::GetMask()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// the mask.
int svtkMaskPointsFilter::FilterPoints(svtkPointSet* input)
{
  // Grab the image data and metadata. The type and existence of image data
  // should have been checked in RequestData().
  double origin[3], spacing[3];
  int dims[3];
  this->Mask->GetDimensions(dims);
  this->Mask->GetOrigin(origin);
  this->Mask->GetSpacing(spacing);
  unsigned char ev = this->EmptyValue;

  unsigned char* m = static_cast<unsigned char*>(this->Mask->GetScalarPointer());

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  svtkDataArray* ptArray = input->GetPoints()->GetData();

  // Use a fast path for double/float points:
  using svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::DispatchByValueType<Reals>;
  ExtractPoints worker;
  if (!Dispatcher::Execute(ptArray, worker, m, ev, dims, origin, spacing, this->PointMap))
  { // fallback for weird types
    worker(ptArray, m, ev, dims, origin, spacing, this->PointMap);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Due to the second input, retrieve it and then invoke the superclass
// RequestData.
int svtkMaskPointsFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* maskInfo = inputVector[1]->GetInformationObject(0);

  // get the mask
  this->Mask = svtkImageData::SafeDownCast(maskInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!this->Mask)
  {
    svtkWarningMacro(<< "No image mask available");
    return 1;
  }

  if (this->Mask->GetScalarType() != SVTK_UNSIGNED_CHAR)
  {
    svtkWarningMacro(<< "Image mask must be unsigned char type");
    return 1;
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkMaskPointsFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* maskInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(maskInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(maskInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // Make sure that the scalar type and number of components
  // are propagated from the mask not the input.
  if (svtkImageData::HasScalarType(maskInfo))
  {
    svtkImageData::SetScalarType(svtkImageData::GetScalarType(maskInfo), outInfo);
  }
  if (svtkImageData::HasNumberOfScalarComponents(maskInfo))
  {
    svtkImageData::SetNumberOfScalarComponents(
      svtkImageData::GetNumberOfScalarComponents(maskInfo), outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkMaskPointsFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* maskInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  maskInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  maskInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  maskInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  maskInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    maskInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
void svtkMaskPointsFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Empty Value: " << this->EmptyValue << "\n";
}
