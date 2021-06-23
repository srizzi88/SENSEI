/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointOccupancyFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointOccupancyFilter.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkPointOccupancyFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Operator() processes templated points.
template <typename T>
struct ComputeOccupancy
{
  T* Points;
  double hX, hY, hZ; // internal data members for performance
  double fX, fY, fZ, bX, bY, bZ;
  svtkIdType xD, yD, zD, xyD;
  unsigned char OccupiedValue;
  unsigned char* Occupancy;

  ComputeOccupancy(T* pts, int dims[3], double origin[3], double spacing[3], unsigned char empty,
    unsigned char occupied, unsigned char* occ)
    : Points(pts)
    , OccupiedValue(occupied)
    , Occupancy(occ)
  {
    std::fill_n(this->Occupancy, dims[0] * dims[1] * dims[2], static_cast<unsigned char>(empty));
    for (int i = 0; i < 3; ++i)
    {
      this->hX = spacing[0];
      this->hY = spacing[1];
      this->hZ = spacing[2];
      this->fX = 1.0 / spacing[0];
      this->fY = 1.0 / spacing[1];
      this->fZ = 1.0 / spacing[2];
      this->bX = origin[0] - 0.5 * this->hX;
      this->bY = origin[1] - 0.5 * this->hY;
      this->bZ = origin[2] - 0.5 * this->hZ;
      this->xD = dims[0];
      this->yD = dims[1];
      this->zD = dims[2];
      this->xyD = dims[0] * dims[1];
    }
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    T* x = this->Points + 3 * ptId;
    unsigned char* o = this->Occupancy;
    unsigned char ov = this->OccupiedValue;
    int i, j, k;

    for (; ptId < endPtId; ++ptId, x += 3)
    {

      i = static_cast<int>(((x[0] - this->bX) * this->fX));
      j = static_cast<int>(((x[1] - this->bY) * this->fY));
      k = static_cast<int>(((x[2] - this->bZ) * this->fZ));

      // If not inside image then skip
      if (i < 0 || i >= this->xD || j < 0 || j >= this->yD || k < 0 || k >= this->zD)
      {
        continue;
      }

      o[i + j * this->xD + k * this->xyD] = ov;

    } // over points
  }

  static void Execute(svtkIdType npts, T* pts, int dims[3], double origin[3], double spacing[3],
    unsigned char ev, unsigned char ov, unsigned char* o)
  {
    ComputeOccupancy compOcc(pts, dims, origin, spacing, ev, ov, o);
    svtkSMPTools::For(0, npts, compOcc);
  }
}; // ComputeOccupancy

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkPointOccupancyFilter::svtkPointOccupancyFilter()
{
  this->SampleDimensions[0] = 100;
  this->SampleDimensions[1] = 100;
  this->SampleDimensions[2] = 100;

  // All of these zeros mean automatic computation
  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;

  this->EmptyValue = 0;
  this->OccupiedValue = 1;
}

//----------------------------------------------------------------------------
svtkPointOccupancyFilter::~svtkPointOccupancyFilter() = default;

//----------------------------------------------------------------------------
int svtkPointOccupancyFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPointOccupancyFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->SampleDimensions[0] - 1,
    0, this->SampleDimensions[1] - 1, 0, this->SampleDimensions[2] - 1);

  for (i = 0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2 * i];
    if (this->SampleDimensions[i] <= 1)
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] =
        (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    }
  }
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), ar, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, 1);

  return 1;
}

//----------------------------------------------------------------------------
// Compute the size of the sample bounding box automatically from the
// input data.
void svtkPointOccupancyFilter::ComputeModelBounds(
  svtkDataSet* input, svtkImageData* output, svtkInformation* outInfo)
{
  int i;

  // compute model bounds if not set previously
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
    this->ModelBounds[2] >= this->ModelBounds[3] || this->ModelBounds[4] >= this->ModelBounds[5])
  {
    input->GetBounds(this->ModelBounds);
  }

  // Set volume origin and data spacing
  outInfo->Set(
    svtkDataObject::ORIGIN(), this->ModelBounds[0], this->ModelBounds[2], this->ModelBounds[4]);
  memcpy(this->Origin, outInfo->Get(svtkDataObject::ORIGIN()), sizeof(double) * 3);
  output->SetOrigin(this->Origin);

  for (i = 0; i < 3; i++)
  {
    this->Spacing[i] =
      (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    if (this->Spacing[i] <= 0.0)
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);
  output->SetSpacing(this->Spacing);
}

//----------------------------------------------------------------------------
// Set the dimensions of the sampling volume
void svtkPointOccupancyFilter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void svtkPointOccupancyFilter::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  svtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2]
                << ")");

  if (dim[0] != this->SampleDimensions[0] || dim[1] != this->SampleDimensions[1] ||
    dim[2] != this->SampleDimensions[2])
  {
    if (dim[0] < 1 || dim[1] < 1 || dim[2] < 1)
    {
      svtkErrorMacro(<< "Bad Sample Dimensions, retaining previous values");
      return;
    }

    for (dataDim = 0, i = 0; i < 3; i++)
    {
      if (dim[i] > 1)
      {
        dataDim++;
      }
    }

    if (dataDim < 3)
    {
      svtkErrorMacro(<< "Sample dimensions must define a volume!");
      return;
    }

    for (i = 0; i < 3; i++)
    {
      this->SampleDimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Produce the output data
int svtkPointOccupancyFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Configure the output
  output->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);
  int* extent = this->GetExecutive()->GetOutputInformation(0)->Get(
    svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(input, output, outInfo);

  // Make sure points are available
  svtkIdType npts = input->GetNumberOfPoints();
  if (npts == 0)
  {
    svtkWarningMacro(<< "No POINTS input!!");
    return 1;
  }

  // Grab the raw point data
  void* pts = input->GetPoints()->GetVoidPointer(0);

  // Grab the occupancy image and process it.
  output->AllocateScalars(outInfo);
  svtkDataArray* occ = output->GetPointData()->GetScalars();
  unsigned char* o = static_cast<unsigned char*>(output->GetArrayPointerForExtent(occ, extent));

  int dims[3];
  double origin[3], spacing[3];
  output->GetDimensions(dims);
  output->GetOrigin(origin);
  output->GetSpacing(spacing);
  unsigned char ev = this->EmptyValue;
  unsigned char ov = this->OccupiedValue;

  switch (input->GetPoints()->GetDataType())
  {
    svtkTemplateMacro(
      ComputeOccupancy<SVTK_TT>::Execute(npts, (SVTK_TT*)pts, dims, origin, spacing, ev, ov, o));
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkPointOccupancyFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
     << this->SampleDimensions[1] << ", " << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Empty Value: " << this->EmptyValue << "\n";
  os << indent << "Occupied Value: " << this->OccupiedValue << "\n";
}
