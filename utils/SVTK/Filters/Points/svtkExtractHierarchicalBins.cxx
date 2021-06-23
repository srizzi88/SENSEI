/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractHierarchicalBins.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractHierarchicalBins.h"

#include "svtkDataArray.h"
#include "svtkGarbageCollector.h"
#include "svtkHierarchicalBinningFilter.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"

svtkStandardNewMacro(svtkExtractHierarchicalBins);
svtkCxxSetObjectMacro(svtkExtractHierarchicalBins, BinningFilter, svtkHierarchicalBinningFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// Mark points to be extracted
static void MaskPoints(svtkIdType numPts, svtkIdType* map, svtkIdType offset, svtkIdType numFill)
{
  std::fill_n(map, offset, static_cast<svtkIdType>(-1));
  std::fill_n(map + offset, numFill, static_cast<svtkIdType>(1));
  std::fill_n(map + offset + numFill, numPts - (offset + numFill), static_cast<svtkIdType>(-1));
}

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkExtractHierarchicalBins::svtkExtractHierarchicalBins()
{
  this->Level = 0;
  this->Bin = -1;
  this->BinningFilter = nullptr;
}

//----------------------------------------------------------------------------
svtkExtractHierarchicalBins::~svtkExtractHierarchicalBins()
{
  this->SetBinningFilter(nullptr);
}

void svtkExtractHierarchicalBins::ReportReferences(svtkGarbageCollector* collector)
{
  // Report references held by this object that may be in a loop.
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->BinningFilter, "Binning Filter");
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// and implicit function.
int svtkExtractHierarchicalBins::FilterPoints(svtkPointSet* input)
{
  // Check the input.
  if (!this->BinningFilter)
  {
    svtkErrorMacro(<< "svtkHierarchicalBinningFilter required\n");
    return 0;
  }

  // Access the correct bin and determine how many points to extract.
  svtkIdType offset;
  svtkIdType numFill;

  if (this->Level >= 0)
  {
    int level = (this->Level < this->BinningFilter->GetNumberOfLevels()
        ? this->Level
        : (this->BinningFilter->GetNumberOfLevels() - 1));
    offset = this->BinningFilter->GetLevelOffset(level, numFill);
  }
  else if (this->Bin >= 0)
  {
    int bin = (this->Level < this->BinningFilter->GetNumberOfGlobalBins()
        ? this->Bin
        : (this->BinningFilter->GetNumberOfGlobalBins() - 1));
    offset = this->BinningFilter->GetBinOffset(bin, numFill);
  }
  else // pass everything through
  {
    return 1;
  }

  svtkIdType numPts = input->GetNumberOfPoints();
  MaskPoints(numPts, this->PointMap, offset, numFill);

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractHierarchicalBins::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Level: " << this->Level << "\n";
  os << indent << "Bin: " << this->Bin << "\n";
  os << indent << "Binning Filter: " << static_cast<void*>(this->BinningFilter) << "\n";
}
