/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResampleToImage.h"

#include "svtkCharArray.h"
#include "svtkCompositeDataProbeFilter.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

#include <algorithm>

svtkObjectFactoryNewMacro(svtkResampleToImage);

//----------------------------------------------------------------------------
svtkResampleToImage::svtkResampleToImage()
  : UseInputBounds(true)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->SamplingBounds[0] = this->SamplingBounds[2] = this->SamplingBounds[4] = 0;
  this->SamplingBounds[1] = this->SamplingBounds[3] = this->SamplingBounds[5] = 1;
  this->SamplingDimensions[0] = this->SamplingDimensions[1] = this->SamplingDimensions[2] = 10;
}

//----------------------------------------------------------------------------
svtkResampleToImage::~svtkResampleToImage() = default;

//----------------------------------------------------------------------------
void svtkResampleToImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseInputBounds " << this->UseInputBounds << endl;
  os << indent << "SamplingBounds [" << this->SamplingBounds[0] << ", " << this->SamplingBounds[1]
     << ", " << this->SamplingBounds[2] << ", " << this->SamplingBounds[3] << ", "
     << this->SamplingBounds[4] << ", " << this->SamplingBounds[5] << "]" << endl;
  os << indent << "SamplingDimensions " << this->SamplingDimensions[0] << " x "
     << this->SamplingDimensions[1] << " x " << this->SamplingDimensions[2] << endl;
}

//----------------------------------------------------------------------------
svtkImageData* svtkResampleToImage::GetOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
svtkTypeBool svtkResampleToImage::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // propagate update extent
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkResampleToImage::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  int wholeExtent[6] = { 0, this->SamplingDimensions[0] - 1, 0, this->SamplingDimensions[1] - 1, 0,
    this->SamplingDimensions[2] - 1 };

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkResampleToImage::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  // This filter always asks for whole extent downstream. To resample
  // a subset of a structured input, you need to use ExtractVOI.
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Remove(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkResampleToImage::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkResampleToImage::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
const char* svtkResampleToImage::GetMaskArrayName() const
{
  return "svtkValidPointMask";
}

//----------------------------------------------------------------------------
namespace
{

inline void ComputeBoundingExtent(
  const double origin[3], const double spacing[3], const double bounds[6], int extent[6])
{
  for (int i = 0; i < 3; ++i)
  {
    if (spacing[i] != 0.0)
    {
      extent[2 * i] = static_cast<int>(svtkMath::Floor((bounds[2 * i] - origin[i]) / spacing[i]));
      extent[2 * i + 1] =
        static_cast<int>(svtkMath::Ceil((bounds[2 * i + 1] - origin[i]) / spacing[i]));
    }
    else
    {
      extent[2 * i] = extent[2 * i + 1] = 0;
    }
  }
}

} // anonymous namespace

void svtkResampleToImage::PerformResampling(svtkDataObject* input, const double samplingBounds[6],
  bool computeProbingExtent, const double inputBounds[6], svtkImageData* output)
{
  if (this->SamplingDimensions[0] <= 0 || this->SamplingDimensions[1] <= 0 ||
    this->SamplingDimensions[2] <= 0)
  {
    return;
  }

  // compute bounds and extent where probing should be performed
  double origin[3] = { samplingBounds[0], samplingBounds[2], samplingBounds[4] };
  double spacing[3];
  for (int i = 0; i < 3; ++i)
  {
    spacing[i] = (this->SamplingDimensions[i] == 1)
      ? 0
      : ((samplingBounds[i * 2 + 1] - samplingBounds[i * 2]) /
          static_cast<double>(this->SamplingDimensions[i] - 1));
  }

  int* updateExtent = this->GetUpdateExtent();
  int probingExtent[6];
  if (computeProbingExtent)
  {
    ComputeBoundingExtent(origin, spacing, inputBounds, probingExtent);
    for (int i = 0; i < 3; ++i)
    {
      probingExtent[2 * i] = svtkMath::Max(probingExtent[2 * i], updateExtent[2 * i]);
      probingExtent[2 * i + 1] = svtkMath::Min(probingExtent[2 * i + 1], updateExtent[2 * i + 1]);
      if (probingExtent[2 * i] > probingExtent[2 * i + 1]) // no overlap
      {
        probingExtent[0] = probingExtent[2] = probingExtent[4] = 0;
        probingExtent[1] = probingExtent[3] = probingExtent[5] = -1;
        break;
      }
    }
  }
  else
  {
    std::copy(updateExtent, updateExtent + 6, probingExtent);
  }

  // perform probing
  svtkNew<svtkImageData> structure;
  structure->SetOrigin(origin);
  structure->SetSpacing(spacing);
  structure->SetExtent(probingExtent);

  svtkNew<svtkCompositeDataProbeFilter> prober;
  prober->SetInputData(structure);
  prober->SetSourceData(input);
  prober->Update();

  output->ShallowCopy(prober->GetOutput());
  output->GetFieldData()->PassData(input->GetFieldData());
}

//----------------------------------------------------------------------------
namespace
{

class MarkHiddenPoints
{
public:
  MarkHiddenPoints(char* maskArray, svtkUnsignedCharArray* pointGhostArray)
    : MaskArray(maskArray)
    , PointGhostArray(pointGhostArray)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    for (svtkIdType i = begin; i < end; ++i)
    {
      if (!this->MaskArray[i])
      {
        this->PointGhostArray->SetValue(
          i, this->PointGhostArray->GetValue(i) | svtkDataSetAttributes::HIDDENPOINT);
      }
    }
  }

private:
  char* MaskArray;
  svtkUnsignedCharArray* PointGhostArray;
};

class MarkHiddenCells
{
public:
  MarkHiddenCells(svtkImageData* data, char* maskArray, svtkUnsignedCharArray* cellGhostArray)
    : Data(data)
    , MaskArray(maskArray)
    , CellGhostArray(cellGhostArray)
  {
    this->Data->GetDimensions(this->PointDim);
    this->PointSliceSize = this->PointDim[0] * this->PointDim[1];

    this->CellDim[0] = svtkMath::Max(1, this->PointDim[0] - 1);
    this->CellDim[1] = svtkMath::Max(1, this->PointDim[1] - 1);
    this->CellDim[2] = svtkMath::Max(1, this->PointDim[2] - 1);
    this->CellSliceSize = this->CellDim[0] * this->CellDim[1];
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    for (svtkIdType cellId = begin; cellId < end; ++cellId)
    {
      int ptijk[3];
      ptijk[2] = cellId / this->CellSliceSize;
      ptijk[1] = (cellId % CellSliceSize) / this->CellDim[0];
      ptijk[0] = (cellId % CellSliceSize) % this->CellDim[0];

      svtkIdType ptid = ptijk[0] + this->PointDim[0] * ptijk[1] + this->PointSliceSize * ptijk[2];

      int dim[3];
      dim[0] = (this->PointDim[0] > 1) ? 1 : 0;
      dim[1] = (this->PointDim[1] > 1) ? 1 : 0;
      dim[2] = (this->PointDim[2] > 1) ? 1 : 0;

      bool validCell = true;
      for (int k = 0; k <= dim[2]; ++k)
      {
        for (int j = 0; j <= dim[1]; ++j)
        {
          for (int i = 0; i <= dim[0]; ++i)
          {
            validCell &= (0 !=
              this->MaskArray[ptid + i + (j * this->PointDim[0]) + (k * this->PointSliceSize)]);
          }
        }
      }

      if (!validCell)
      {
        this->CellGhostArray->SetValue(
          cellId, this->CellGhostArray->GetValue(cellId) | svtkDataSetAttributes::HIDDENCELL);
      }
    }
  }

private:
  svtkImageData* Data;
  char* MaskArray;
  svtkUnsignedCharArray* CellGhostArray;

  int PointDim[3];
  svtkIdType PointSliceSize;
  int CellDim[3];
  svtkIdType CellSliceSize;
};

} // anonymous namespace

void svtkResampleToImage::SetBlankPointsAndCells(svtkImageData* data)
{
  if (data->GetNumberOfPoints() <= 0)
  {
    return;
  }

  svtkPointData* pd = data->GetPointData();
  svtkCharArray* maskArray = svtkArrayDownCast<svtkCharArray>(pd->GetArray(this->GetMaskArrayName()));
  char* mask = maskArray->GetPointer(0);

  data->AllocatePointGhostArray();
  svtkUnsignedCharArray* pointGhostArray = data->GetPointGhostArray();

  svtkIdType numPoints = data->GetNumberOfPoints();
  MarkHiddenPoints pointWorklet(mask, pointGhostArray);
  svtkSMPTools::For(0, numPoints, pointWorklet);

  data->AllocateCellGhostArray();
  svtkUnsignedCharArray* cellGhostArray = data->GetCellGhostArray();

  svtkIdType numCells = data->GetNumberOfCells();
  MarkHiddenCells cellWorklet(data, mask, cellGhostArray);
  svtkSMPTools::For(0, numCells, cellWorklet);
}

//----------------------------------------------------------------------------
int svtkResampleToImage::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  double samplingBounds[6];
  if (this->UseInputBounds)
  {
    ComputeDataBounds(input, samplingBounds);
  }
  else
  {
    std::copy(this->SamplingBounds, this->SamplingBounds + 6, samplingBounds);
  }

  this->PerformResampling(input, samplingBounds, false, nullptr, output);
  this->SetBlankPointsAndCells(output);

  return 1;
}

//----------------------------------------------------------------------------
void svtkResampleToImage::ComputeDataBounds(svtkDataObject* data, double bounds[6])
{
  if (svtkDataSet::SafeDownCast(data))
  {
    svtkDataSet::SafeDownCast(data)->GetBounds(bounds);
  }
  else
  {
    svtkCompositeDataSet* cdata = svtkCompositeDataSet::SafeDownCast(data);
    bounds[0] = bounds[2] = bounds[4] = SVTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] = -SVTK_DOUBLE_MAX;

    using Opts = svtk::CompositeDataSetOptions;
    for (svtkDataObject* dObj : svtk::Range(cdata, Opts::SkipEmptyNodes))
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(dObj);
      if (!ds)
      {
        svtkGenericWarningMacro("svtkCompositeDataSet leaf not svtkDataSet. Skipping.");
        continue;
      }
      double b[6];
      ds->GetBounds(b);
      for (int i = 0; i < 3; ++i)
      {
        bounds[2 * i] = svtkMath::Min(bounds[2 * i], b[2 * i]);
        bounds[2 * i + 1] = svtkMath::Max(bounds[2 * i + 1], b[2 * i + 1]);
      }
    }
  }
}
