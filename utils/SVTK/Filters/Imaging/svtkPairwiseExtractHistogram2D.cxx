/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPairwiseExtractHistogram2D.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/

#include "svtkPairwiseExtractHistogram2D.h"
//------------------------------------------------------------------------------
#include "svtkArrayData.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkCollection.h"
#include "svtkExtractHistogram2D.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageMedian3D.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedIntArray.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()
//------------------------------------------------------------------------------
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkPairwiseExtractHistogram2D);
//------------------------------------------------------------------------------
class svtkPairwiseExtractHistogram2D::Internals
{
public:
  std::vector<std::pair<svtkStdString, svtkStdString> > ColumnPairs;
  std::map<std::string, bool> ColumnUsesCustomExtents;
  std::map<std::string, std::vector<double> > ColumnExtents;
};
//------------------------------------------------------------------------------
svtkPairwiseExtractHistogram2D::svtkPairwiseExtractHistogram2D()
{
  this->Implementation = new Internals;

  this->SetNumberOfOutputPorts(4);

  this->NumberOfBins[0] = 0;
  this->NumberOfBins[1] = 0;

  this->CustomColumnRangeIndex = -1;

  this->ScalarType = SVTK_UNSIGNED_INT;
  this->HistogramFilters = svtkSmartPointer<svtkCollection>::New();
  this->BuildTime.Modified();
}
//------------------------------------------------------------------------------
svtkPairwiseExtractHistogram2D::~svtkPairwiseExtractHistogram2D()
{
  delete this->Implementation;
}
//------------------------------------------------------------------------------
void svtkPairwiseExtractHistogram2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfBins: " << this->NumberOfBins[0] << ", " << this->NumberOfBins[1] << endl;
  os << "CustomColumnRangeIndex: " << this->CustomColumnRangeIndex << endl;
  os << "ScalarType: " << this->ScalarType << endl;
}
//------------------------------------------------------------------------------
void svtkPairwiseExtractHistogram2D::Learn(
  svtkTable* inData, svtkTable* svtkNotUsed(inParameters), svtkMultiBlockDataSet* outMeta)
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

  // The primary statistics table
  svtkTable* primaryTab = svtkTable::New();

  // if the number of columns in the input has changed, we'll need to do
  // some reinitializing
  int numHistograms = inData->GetNumberOfColumns() - 1;
  if (numHistograms != this->HistogramFilters->GetNumberOfItems())
  {
    // clear out the list of histogram filters
    for (int i = 0; i < this->HistogramFilters->GetNumberOfItems(); i++)
    {
      this->HistogramFilters->GetItemAsObject(i)->Delete();
    }

    // clear out the internals
    this->HistogramFilters->RemoveAllItems();
    this->Implementation->ColumnPairs.clear();
    this->Implementation->ColumnUsesCustomExtents.clear();
    this->Implementation->ColumnExtents.clear();

    // Make a shallow copy of the input to be safely passed to internal
    // histogram filters.
    SVTK_CREATE(svtkTable, inDataCopy);
    inDataCopy->ShallowCopy(inData);

    // fill it up with new histogram filters
    for (int i = 0; i < numHistograms; i++)
    {
      svtkDataArray* col1 = svtkArrayDownCast<svtkDataArray>(inData->GetColumn(i));
      svtkDataArray* col2 = svtkArrayDownCast<svtkDataArray>(inData->GetColumn(i + 1));

      if (!col1 || !col2)
      {
        svtkErrorMacro("All inputs must be numeric arrays.");
        return;
      }

      // create a new histogram filter
      svtkSmartPointer<svtkExtractHistogram2D> f;
      f.TakeReference(this->NewHistogramFilter());
      f->SetInputData(inDataCopy);
      f->SetNumberOfBins(this->NumberOfBins);
      std::pair<svtkStdString, svtkStdString> colpair(
        inData->GetColumn(i)->GetName(), inData->GetColumn(i + 1)->GetName());
      f->AddColumnPair(colpair.first.c_str(), colpair.second.c_str());
      f->SetSwapColumns(strcmp(colpair.first.c_str(), colpair.second.c_str()) >= 0);
      this->HistogramFilters->AddItem(f);

      // update the internals accordingly
      this->Implementation->ColumnPairs.push_back(colpair);
      this->Implementation->ColumnUsesCustomExtents[colpair.first.c_str()] = false;

      // compute the range of the new columns, and update the internals
      double r[2] = { 0, 0 };
      if (i == 0)
      {
        col1->GetRange(r, 0);
        this->Implementation->ColumnExtents[colpair.first.c_str()].clear();
        this->Implementation->ColumnExtents[colpair.first.c_str()].push_back(r[0]);
        this->Implementation->ColumnExtents[colpair.first.c_str()].push_back(r[1]);
      }

      col2->GetRange(r, 0);
      this->Implementation->ColumnExtents[colpair.second.c_str()].clear();
      this->Implementation->ColumnExtents[colpair.second.c_str()].push_back(r[0]);
      this->Implementation->ColumnExtents[colpair.second.c_str()].push_back(r[1]);
    }
  }

  // check the filters one by one and update them if necessary
  if (this->BuildTime < inData->GetMTime() || this->BuildTime < this->GetMTime())
  {
    for (int i = 0; i < numHistograms; i++)
    {

      svtkExtractHistogram2D* f = this->GetHistogramFilter(i);

      // if the column names have changed, that means we need to update
      std::pair<svtkStdString, svtkStdString> cols = this->Implementation->ColumnPairs[i];
      if (inData->GetColumn(i)->GetName() != cols.first ||
        inData->GetColumn(i + 1)->GetName() != cols.second)
      {
        std::pair<svtkStdString, svtkStdString> newCols(
          inData->GetColumn(i)->GetName(), inData->GetColumn(i + 1)->GetName());

        f->ResetRequests();
        f->AddColumnPair(newCols.first.c_str(), newCols.second.c_str());
        f->SetSwapColumns(strcmp(newCols.first.c_str(), newCols.second.c_str()) >= 0);
        f->Modified();

        this->Implementation->ColumnPairs[i] = newCols;
      }

      // if the filter extents have changed, that means we need to update
      std::pair<svtkStdString, svtkStdString> newCols(
        inData->GetColumn(i)->GetName(), inData->GetColumn(i + 1)->GetName());
      if (this->Implementation->ColumnUsesCustomExtents[newCols.first.c_str()] ||
        this->Implementation->ColumnUsesCustomExtents[newCols.second.c_str()])
      {
        f->UseCustomHistogramExtentsOn();
        double* ext = f->GetCustomHistogramExtents();
        if (ext[0] != this->Implementation->ColumnExtents[newCols.first.c_str()][0] ||
          ext[1] != this->Implementation->ColumnExtents[newCols.first.c_str()][1] ||
          ext[2] != this->Implementation->ColumnExtents[newCols.second.c_str()][0] ||
          ext[3] != this->Implementation->ColumnExtents[newCols.second.c_str()][1])
        {
          f->SetCustomHistogramExtents(
            this->Implementation->ColumnExtents[newCols.first.c_str()][0],
            this->Implementation->ColumnExtents[newCols.first.c_str()][1],
            this->Implementation->ColumnExtents[newCols.second.c_str()][0],
            this->Implementation->ColumnExtents[newCols.second.c_str()][1]);
        }
      }
      else
      {
        f->UseCustomHistogramExtentsOff();
      }

      // if the number of bins has changed, that definitely means we need to update
      int* nbins = f->GetNumberOfBins();
      if (nbins[0] != this->NumberOfBins[0] || nbins[1] != this->NumberOfBins[1])
      {
        f->SetNumberOfBins(this->NumberOfBins);
      }
    }
  }

  // update the filters as necessary
  for (int i = 0; i < numHistograms; i++)
  {
    svtkExtractHistogram2D* f = this->GetHistogramFilter(i);
    if (f &&
      (f->GetMTime() > this->BuildTime || inData->GetColumn(i)->GetMTime() > this->BuildTime ||
        inData->GetColumn(i + 1)->GetMTime() > this->BuildTime))
    {
      f->Update();
    }
  }

  // build the composite image data set
  svtkMultiBlockDataSet* outImages = svtkMultiBlockDataSet::SafeDownCast(
    this->GetOutputDataObject(svtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE));

  if (outImages)
  {
    outImages->SetNumberOfBlocks(numHistograms);
    for (int i = 0; i < numHistograms; i++)
    {
      svtkExtractHistogram2D* f = this->GetHistogramFilter(i);
      if (f)
      {
        outImages->SetBlock(i, f->GetOutputHistogramImage());
      }
    }
  }

  // build the output table
  primaryTab->Initialize();
  for (int i = 0; i < this->HistogramFilters->GetNumberOfItems(); i++)
  {
    svtkExtractHistogram2D* f = this->GetHistogramFilter(i);
    if (f)
    {
      if (f->GetMTime() > this->BuildTime)
        f->Update();
      primaryTab->AddColumn(f->GetOutput()->GetColumn(0));
    }
  }

  // Finally set first block of output meta port to primary statistics table
  outMeta->SetNumberOfBlocks(1);
  outMeta->GetMetaData(static_cast<unsigned>(0))
    ->Set(svtkCompositeDataSet::NAME(), "Primary Statistics");
  outMeta->SetBlock(0, primaryTab);

  // Clean up
  primaryTab->Delete();

  this->BuildTime.Modified();
}
//------------------------------------------------------------------------------
void svtkPairwiseExtractHistogram2D::SetCustomColumnRangeByIndex(double rmin, double rmax)
{
  this->SetCustomColumnRange(this->CustomColumnRangeIndex, rmin, rmax);
}
//------------------------------------------------------------------------------
void svtkPairwiseExtractHistogram2D::SetCustomColumnRange(int column, double rmin, double rmax)
{
  svtkTable* t = svtkTable::SafeDownCast(this->GetInputDataObject(0, 0));
  if (t)
  {
    svtkAbstractArray* a = t->GetColumn(column);
    if (a)
    {
      this->Implementation->ColumnUsesCustomExtents[a->GetName()] = true;
      if (this->Implementation->ColumnExtents[a->GetName()].empty())
      {
        this->Implementation->ColumnExtents[a->GetName()].push_back(rmin);
        this->Implementation->ColumnExtents[a->GetName()].push_back(rmax);
      }
      else
      {
        this->Implementation->ColumnExtents[a->GetName()][0] = rmin;
        this->Implementation->ColumnExtents[a->GetName()][1] = rmax;
      }
      this->Modified();
    }
  }
}
//------------------------------------------------------------------------------
void svtkPairwiseExtractHistogram2D::SetCustomColumnRange(int column, double range[2])
{
  this->SetCustomColumnRange(column, range[0], range[1]);
}
//------------------------------------------------------------------------------
int svtkPairwiseExtractHistogram2D::GetBinRange(
  int idx, svtkIdType binX, svtkIdType binY, double range[4])
{
  svtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
  {
    return f->GetBinRange(binX, binY, range);
  }
  else
  {
    return 0;
  }
}

int svtkPairwiseExtractHistogram2D::GetBinRange(int idx, svtkIdType bin, double range[4])
{
  svtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
  {
    return f->GetBinRange(bin, range);
  }
  else
  {
    return 0;
  }
}
//------------------------------------------------------------------------------
svtkExtractHistogram2D* svtkPairwiseExtractHistogram2D::GetHistogramFilter(int idx)
{
  return svtkExtractHistogram2D::SafeDownCast(this->HistogramFilters->GetItemAsObject(idx));
}
//------------------------------------------------------------------------------
svtkImageData* svtkPairwiseExtractHistogram2D::GetOutputHistogramImage(int idx)
{
  if (this->BuildTime < this->GetMTime() ||
    this->BuildTime < this->GetInputDataObject(0, 0)->GetMTime())
    this->Update();

  svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(
    this->GetOutputDataObject(svtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE));

  if (mbds)
  {
    return svtkImageData::SafeDownCast(mbds->GetBlock(idx));
  }
  return nullptr;
}
//------------------------------------------------------------------------------
void svtkPairwiseExtractHistogram2D::GetBinWidth(int idx, double bw[2])
{
  svtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
  {
    f->GetBinWidth(bw);
  }
}
//------------------------------------------------------------------------------
double* svtkPairwiseExtractHistogram2D::GetHistogramExtents(int idx)
{
  svtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
  {
    return f->GetHistogramExtents();
  }
  else
  {
    return nullptr;
  }
}
//------------------------------------------------------------------------------
svtkExtractHistogram2D* svtkPairwiseExtractHistogram2D::NewHistogramFilter()
{
  return svtkExtractHistogram2D::New();
}
//------------------------------------------------------------------------------
double svtkPairwiseExtractHistogram2D::GetMaximumBinCount(int idx)
{
  svtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
  {
    return f->GetMaximumBinCount();
  }
  return -1;
}
//------------------------------------------------------------------------------
double svtkPairwiseExtractHistogram2D::GetMaximumBinCount()
{
  if (!this->GetInputDataObject(0, 0))
    return -1;

  if (this->BuildTime < this->GetMTime() ||
    this->BuildTime < this->GetInputDataObject(0, 0)->GetMTime())
    this->Update();

  double maxcount = -1;
  for (int i = 0; i < this->HistogramFilters->GetNumberOfItems(); i++)
  {
    svtkExtractHistogram2D* f = this->GetHistogramFilter(i);
    if (f)
    {
      maxcount = std::max(f->GetMaximumBinCount(), maxcount);
    }
  }
  return maxcount;
}
//------------------------------------------------------------------------------
int svtkPairwiseExtractHistogram2D::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == svtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
    return 1;
  }
  else
  {
    return this->Superclass::FillOutputPortInformation(port, info);
  }
}
