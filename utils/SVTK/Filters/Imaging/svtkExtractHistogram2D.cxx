/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkExtractHistogram2D.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkExtractHistogram2D.h"
//------------------------------------------------------------------------------
#include "svtkArrayData.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageMedian3D.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedIntArray.h"
//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkExtractHistogram2D);
svtkCxxSetObjectMacro(svtkExtractHistogram2D, RowMask, svtkDataArray);
//------------------------------------------------------------------------------
// Figure out which histogram bin a pair of values fit into
static inline int svtkExtractHistogram2DComputeBin(
  svtkIdType& bin1, svtkIdType& bin2, double v1, double v2, double* exts, int* nbins, double* bwi)
{
  // Make they fit within the extents
  if (v1 < exts[0] || v1 > exts[1] || v2 < exts[2] || v2 > exts[3])
    return 0;

  // as usual, boundary cases are annoying
  bin1 = (v1 == exts[1]) ? nbins[0] - 1 : static_cast<svtkIdType>(floor((v1 - exts[0]) * bwi[0]));

  bin2 = (v2 == exts[3]) ? nbins[1] - 1 : static_cast<svtkIdType>(floor((v2 - exts[2]) * bwi[1]));
  return 1;
}
//------------------------------------------------------------------------------
svtkExtractHistogram2D::svtkExtractHistogram2D()
{
  this->SetNumberOfOutputPorts(4);

  this->NumberOfBins[0] = 0;
  this->NumberOfBins[1] = 0;

  this->HistogramExtents[0] = 0.0;
  this->HistogramExtents[1] = 0.0;
  this->HistogramExtents[2] = 0.0;
  this->HistogramExtents[3] = 0.0;

  this->CustomHistogramExtents[0] = 0.0;
  this->CustomHistogramExtents[1] = 0.0;
  this->CustomHistogramExtents[2] = 0.0;
  this->CustomHistogramExtents[3] = 0.0;

  this->ComponentsToProcess[0] = 0;
  this->ComponentsToProcess[1] = 0;

  this->UseCustomHistogramExtents = 0;
  this->MaximumBinCount = 0;
  this->ScalarType = SVTK_UNSIGNED_INT;

  this->SwapColumns = 0;

  this->RowMask = nullptr;
}
//------------------------------------------------------------------------------
svtkExtractHistogram2D::~svtkExtractHistogram2D()
{
  if (this->RowMask)
    this->RowMask->Delete();
}
//------------------------------------------------------------------------------
void svtkExtractHistogram2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalarType: " << this->ScalarType << endl;
  os << indent << "ComponentsToProcess: " << this->ComponentsToProcess[0] << ", "
     << this->ComponentsToProcess[1] << endl;
  os << indent << "UseCustomHistogramExtents: " << this->UseCustomHistogramExtents << endl;
  os << indent << "MaximumBinCount: " << this->MaximumBinCount << endl;
  os << indent << "SwapColumns: " << this->SwapColumns << endl;
  os << indent << "NumberOfBins: " << this->NumberOfBins[0] << ", " << this->NumberOfBins[1]
     << endl;
  os << indent << "CustomHistogramExtents: " << this->CustomHistogramExtents[0] << ", "
     << this->CustomHistogramExtents[1] << ", " << this->CustomHistogramExtents[2] << ", "
     << this->CustomHistogramExtents[3] << endl;
  os << indent << "RowMask: " << this->RowMask << endl;
}
//------------------------------------------------------------------------------
void svtkExtractHistogram2D::Learn(
  svtkTable* svtkNotUsed(inData), svtkTable* svtkNotUsed(inParameters), svtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  if (this->NumberOfBins[0] == 0 || this->NumberOfBins[1] == 0)
  {
    svtkErrorMacro(<< "Error: histogram dimensions not set (use SetNumberOfBins).");
    return;
  }

  svtkImageData* outImage =
    svtkImageData::SafeDownCast(this->GetOutputDataObject(svtkExtractHistogram2D::HISTOGRAM_IMAGE));

  svtkDataArray *col1 = nullptr, *col2 = nullptr;
  if (!this->GetInputArrays(col1, col2))
  {
    return;
  }

  this->ComputeBinExtents(col1, col2);

  // The primary statistics table
  svtkTable* primaryTab = svtkTable::New();

  int numValues = col1->GetNumberOfTuples();
  if (numValues != col2->GetNumberOfTuples())
  {
    svtkErrorMacro(<< "Error: columns must have same length.");
    return;
  }

  // compute the bin width
  double binWidth[2] = { 0.0, 0.0 };
  this->GetBinWidth(binWidth);

  // allocate the output image
  // svtkImageData is already smart about allocating arrays, so we'll just
  // let it take care of that for us.
  outImage->Initialize();
  outImage->SetExtent(0, this->NumberOfBins[0] - 1, 0, this->NumberOfBins[1] - 1, 0, 0);
  outImage->SetSpacing(binWidth[0], binWidth[1], 0.0);

  outImage->AllocateScalars(this->ScalarType, 1);

  outImage->GetPointData()->GetScalars()->FillComponent(0, 0);
  outImage->GetPointData()->GetScalars()->SetName("histogram");

  svtkDataArray* histogram = outImage->GetPointData()->GetScalars();
  if (!histogram)
  {
    svtkErrorMacro(<< "Error: histogram array not allocated.");
    return;
  }

  svtkIdType bin1, bin2, idx;
  double v1, v2, ct;
  double bwi[2] = { 1.0 / binWidth[0], 1.0 / binWidth[1] };

  bool useRowMask =
    this->RowMask && this->RowMask->GetNumberOfTuples() == col1->GetNumberOfTuples();

  // compute the histogram.
  this->MaximumBinCount = 0;
  for (int i = 0; i < numValues; i++)
  {
    v1 = col1->GetComponent(i, this->ComponentsToProcess[0]);
    v2 = col2->GetComponent(i, this->ComponentsToProcess[1]);

    if (useRowMask && !this->RowMask->GetComponent(i, 0))
      continue;

    if (!::svtkExtractHistogram2DComputeBin(
          bin1, bin2, v1, v2, this->GetHistogramExtents(), this->NumberOfBins, bwi))
      continue;

    idx = (bin1 + this->NumberOfBins[0] * bin2);

    ct = histogram->GetComponent(idx, 0) + 1;
    histogram->SetComponent(idx, 0, ct);

    if (ct > this->MaximumBinCount)
    {
      this->MaximumBinCount = static_cast<svtkIdType>(ct);
    }
  }

  primaryTab->Initialize();
  primaryTab->AddColumn(histogram);

  // Finally set first block of output meta port to primary statistics table
  outMeta->SetNumberOfBlocks(1);
  outMeta->GetMetaData(static_cast<unsigned>(0))
    ->Set(svtkCompositeDataSet::NAME(), "Primary Statistics");
  outMeta->SetBlock(0, primaryTab);

  // Clean up
  primaryTab->Delete();
}

//------------------------------------------------------------------------------
int svtkExtractHistogram2D::GetBinRange(svtkIdType binX, svtkIdType binY, double range[4])
{
  double binWidth[2] = { 0.0, 0.0 };
  double* ext = this->GetHistogramExtents();
  this->GetBinWidth(binWidth);

  range[0] = ext[0] + binX * binWidth[0];
  range[1] = ext[0] + (binX + 1) * binWidth[0];

  range[2] = ext[2] + binY * binWidth[1];
  range[3] = ext[2] + (binY + 1) * binWidth[1];
  return 1;
}
//------------------------------------------------------------------------------
int svtkExtractHistogram2D::GetBinRange(svtkIdType bin, double range[4])
{
  svtkIdType binX = bin % this->NumberOfBins[0];
  svtkIdType binY = bin / this->NumberOfBins[0];
  return this->GetBinRange(binX, binY, range);
}
//------------------------------------------------------------------------------
svtkImageData* svtkExtractHistogram2D::GetOutputHistogramImage()
{
  return svtkImageData::SafeDownCast(
    this->GetOutputDataObject(svtkExtractHistogram2D::HISTOGRAM_IMAGE)); // this->OutputImage;
}
//------------------------------------------------------------------------------
int svtkExtractHistogram2D::GetInputArrays(svtkDataArray*& col1, svtkDataArray*& col2)
{
  svtkTable* inData = svtkTable::SafeDownCast(this->GetInputDataObject(0, 0));

  if (!inData)
  {
    svtkErrorMacro(<< "Error: Empty input.");
    return 0;
  }

  if (!this->Internals->Requests.empty())
  {
    svtkStdString colName;

    this->Internals->GetColumnForRequest(0, (this->SwapColumns != 0), colName);
    col1 = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colName));

    this->Internals->GetColumnForRequest(0, (this->SwapColumns == 0), colName);
    col2 = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colName));
  }
  else
  {
    col1 = svtkArrayDownCast<svtkDataArray>(inData->GetColumn(0));
    col2 = svtkArrayDownCast<svtkDataArray>(inData->GetColumn(1));
  }

  if (!col2)
    col2 = col1;

  if (!col1)
  {
    svtkErrorMacro(<< "Error: could not find first column.");
    return 0;
  }

  if (!col2)
  {
    svtkErrorMacro(<< "Error: could not find second column.");
    return 0;
  }

  if (col1->GetNumberOfComponents() <= this->ComponentsToProcess[0])
  {
    svtkErrorMacro(<< "Error: first column doesn't contain component "
                  << this->ComponentsToProcess[0] << ".");
    return 0;
  }

  if (col2->GetNumberOfComponents() <= this->ComponentsToProcess[1])
  {
    svtkErrorMacro(<< "Error: second column doesn't contain component "
                  << this->ComponentsToProcess[1] << ".");
    return 0;
  }

  return 1;
}
//------------------------------------------------------------------------------
void svtkExtractHistogram2D::GetBinWidth(double bw[2])
{
  double* ext = this->GetHistogramExtents();
  bw[0] = (ext[1] - ext[0]) / static_cast<double>(this->NumberOfBins[0]);
  bw[1] = (ext[3] - ext[2]) / static_cast<double>(this->NumberOfBins[1]);
}
//------------------------------------------------------------------------------
double* svtkExtractHistogram2D::GetHistogramExtents()
{
  if (this->UseCustomHistogramExtents)
    return this->CustomHistogramExtents;
  else
    return this->HistogramExtents;
}
//------------------------------------------------------------------------------
int svtkExtractHistogram2D::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == svtkExtractHistogram2D::HISTOGRAM_IMAGE)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    return 1;
  }
  else
  {
    return this->Superclass::FillOutputPortInformation(port, info);
  }
}

//------------------------------------------------------------------------------
// cribbed from svtkImageReader2
int svtkExtractHistogram2D::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo =
    outputVector->GetInformationObject(svtkExtractHistogram2D::HISTOGRAM_IMAGE);

  svtkDataArray *col1 = nullptr, *col2 = nullptr;
  if (!this->GetInputArrays(col1, col2))
  {
    return 0;
  }

  this->ComputeBinExtents(col1, col2);

  double bw[2] = { 0, 0 };
  double* hext = this->GetHistogramExtents();
  this->GetBinWidth(bw);

  int ext[6] = { 0, this->NumberOfBins[0] - 1, 0, this->NumberOfBins[1] - 1, 0, 0 };
  double sp[3] = { bw[0], bw[1], 0.0 };
  double o[3] = { hext[0], hext[2], 0.0 };
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(svtkDataObject::SPACING(), sp, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), o, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->ScalarType, 1);
  return 1;
}
//------------------------------------------------------------------------------
int svtkExtractHistogram2D::ComputeBinExtents(svtkDataArray* col1, svtkDataArray* col2)
{
  if (!col1 || !col2)
    return 0;

  // update histogram extents, if necessary
  if (!this->UseCustomHistogramExtents)
  {
    col1->GetRange(this->HistogramExtents, this->ComponentsToProcess[0]);
    col2->GetRange(this->HistogramExtents + 2, this->ComponentsToProcess[1]);
  }

  return 1;
}
