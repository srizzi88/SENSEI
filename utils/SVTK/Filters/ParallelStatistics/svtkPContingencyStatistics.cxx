/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPContingencyStatistics.cxx

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

#include "svtkToolkits.h"

#include "svtkPContingencyStatistics.h"

#include "svtkCommunicator.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <map>
#include <set>
#include <vector>

// For debugging purposes, output message sizes and intermediate timings
#define DEBUG_PARALLEL_CONTINGENCY_STATISTICS 0

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
#include "svtkTimerLog.h"
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

svtkStandardNewMacro(svtkPContingencyStatistics);
svtkCxxSetObjectMacro(svtkPContingencyStatistics, Controller, svtkMultiProcessController);
//-----------------------------------------------------------------------------
svtkPContingencyStatistics::svtkPContingencyStatistics()
{
  this->Controller = 0;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
svtkPContingencyStatistics::~svtkPContingencyStatistics()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void svtkPContingencyStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
static void StringVectorToStringBuffer(
  const std::vector<svtkStdString>& strings, svtkStdString& buffer)
{
  buffer.clear();

  for (std::vector<svtkStdString>::const_iterator it = strings.begin(); it != strings.end(); ++it)
  {
    buffer.append(*it);
    buffer.push_back(0);
  }
}

// ----------------------------------------------------------------------
static bool StringArrayToStringBuffer(
  svtkTable* contingencyTab, svtkStdString& xyPacked, std::vector<svtkIdType>& kcValues)
{
  // Downcast meta columns to string arrays for efficient data access
  svtkIdTypeArray* keys = svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Key"));
  svtkAbstractArray* valx = contingencyTab->GetColumnByName("x");
  svtkAbstractArray* valy = contingencyTab->GetColumnByName("y");
  svtkIdTypeArray* card =
    svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Cardinality"));
  if (!keys || !valx || !valy || !card)
  {
    return true;
  }

  std::vector<svtkStdString> xyValues; // consecutive (x,y) pairs

  svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  for (svtkIdType r = 1; r < nRowCont;
       ++r) // Skip first row which is reserved for data set cardinality
  {
    // Push back x and y to list of strings
    xyValues.push_back(valx->GetVariantValue(r).ToString());
    xyValues.push_back(valy->GetVariantValue(r).ToString());

    // Push back (X,Y) index and #(x,y) to list of strings
    kcValues.push_back(keys->GetValue(r));
    kcValues.push_back(card->GetValue(r));
  }

  // Concatenate vector of strings into single string
  StringVectorToStringBuffer(xyValues, xyPacked);

  return false;
}

//-----------------------------------------------------------------------------
static void StringBufferToStringVector(
  const svtkStdString& buffer, std::vector<svtkStdString>& strings)
{
  strings.clear();

  const char* const bufferEnd = &buffer[0] + buffer.size();

  for (const char* start = &buffer[0]; start != bufferEnd; ++start)
  {
    for (const char* finish = start; finish != bufferEnd; ++finish)
    {
      if (!*finish)
      {
        strings.push_back(svtkStdString(start));
        start = finish;
        break;
      }
    }
  }
}

// ----------------------------------------------------------------------
void svtkPContingencyStatistics::Learn(
  svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta)
{
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  svtkTimerLog* timer = svtkTimerLog::New();
  timer->StartTimer();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  if (!outMeta)
  {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
    timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
  }

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  svtkTimerLog* timers = svtkTimerLog::New();
  timers->StartTimer();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  // First calculate contingency statistics on local data set
  this->Superclass::Learn(inData, inParameters, outMeta);
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timers->StopTimer();

  cout << "## Process " << this->Controller->GetCommunicator()->GetLocalProcessId()
       << " serial engine executed in " << timers->GetElapsedTime() << " seconds."
       << "\n";

  timers->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  // Get a hold of the summary table
  svtkTable* summaryTab = svtkTable::SafeDownCast(outMeta->GetBlock(0));
  if (!summaryTab)
  {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
    timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
  }

  // Determine how many (X,Y) variable pairs are present
  svtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  if (nRowSumm <= 0)
  {
    // No statistics were calculated in serial.
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
    timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
  }

  // Get a hold of the contingency table
  svtkTable* contingencyTab = svtkTable::SafeDownCast(outMeta->GetBlock(1));
  if (!contingencyTab)
  {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
    timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
  }

  // Determine number of (x,y) realizations are present
  svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  if (nRowCont <= 0)
  {
    // No statistics were calculated in serial.
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
    timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
  }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
    timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
  }

  // Get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkErrorMacro("No parallel communicator.");
  }

  svtkIdType myRank = com->GetLocalProcessId();

  // Packing step: concatenate all (x,y) pairs in a single string and all (k,c) pairs in single
  // vector
  svtkStdString xyPacked_l;
  std::vector<svtkIdType> kcValues_l;
  if (StringArrayToStringBuffer(contingencyTab, xyPacked_l, kcValues_l))
  {
    svtkErrorMacro("Packing error on process " << myRank << ".");

    return;
  }

  // NB: Use process 0 as sole reducer for now
  svtkIdType rProc = 0;

  // (All) gather all xy and kc sizes
  svtkIdType xySize_l = static_cast<svtkIdType>(xyPacked_l.size());
  svtkIdType* xySize_g = new svtkIdType[np];

  svtkIdType kcSize_l = static_cast<svtkIdType>(kcValues_l.size());
  svtkIdType* kcSize_g = new svtkIdType[np];

  com->AllGather(&xySize_l, xySize_g, 1);

  com->AllGather(&kcSize_l, kcSize_g, 1);

  // Calculate total size and displacement arrays
  svtkIdType* xyOffset = new svtkIdType[np];
  svtkIdType* kcOffset = new svtkIdType[np];

  svtkIdType xySizeTotal = 0;
  svtkIdType kcSizeTotal = 0;

  for (svtkIdType i = 0; i < np; ++i)
  {
    xyOffset[i] = xySizeTotal;
    kcOffset[i] = kcSizeTotal;

    xySizeTotal += xySize_g[i];
    kcSizeTotal += kcSize_g[i];
  }

  // Allocate receive buffers on reducer process, based on the global sizes obtained above
  char* xyPacked_g = 0;
  svtkIdType* kcValues_g = 0;
  if (myRank == rProc)
  {
    xyPacked_g = new char[xySizeTotal];
    kcValues_g = new svtkIdType[kcSizeTotal];
  }

  // Gather all xyPacked and kcValues on process rProc
  // NB: GatherV because the packets have variable lengths
  if (!com->GatherV(&(*xyPacked_l.begin()), xyPacked_g, xySize_l, xySize_g, xyOffset, rProc))
  {
    svtkErrorMacro("Process " << myRank << "could not gather (x,y) values.");

    delete[] xyOffset;
    delete[] kcOffset;
    delete[] xyPacked_g;
    delete[] kcValues_g;
    return;
  }

  if (!com->GatherV(&(*kcValues_l.begin()), kcValues_g, kcSize_l, kcSize_g, kcOffset, rProc))
  {
    svtkErrorMacro("Process " << myRank << "could not gather (k,c) values.");

    delete[] xyOffset;
    delete[] kcOffset;
    delete[] xyPacked_g;
    delete[] kcValues_g;
    return;
  }

  // Reduce to global contingency table on process rProc
  if (myRank == rProc)
  {
    if (this->Reduce(xySizeTotal, xyPacked_g, xyPacked_l, kcSizeTotal, kcValues_g, kcValues_l))
    {
      delete[] xyOffset;
      delete[] kcOffset;
      delete[] xyPacked_g;
      delete[] kcValues_g;
      return;
    }
  } // if ( myRank == rProc )

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  svtkTimerLog* timerB = svtkTimerLog::New();
  timerB->StartTimer();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  // Broadcasting step: broadcast reduced contingency table to all processes
  std::vector<svtkStdString> xyValues_l; // local consecutive xy pairs
  if (this->Broadcast(xySizeTotal, xyPacked_l, xyValues_l, kcSizeTotal, kcValues_l, rProc))
  {
    delete[] xyOffset;
    delete[] kcOffset;
    delete[] xyPacked_g;
    delete[] kcValues_g;
    return;
  }

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timerB->StopTimer();

  cout << "## Process " << myRank << " broadcasted in " << timerB->GetElapsedTime() << " seconds."
       << "\n";

  timerB->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  // Finally, fill the new, global contingency table (everyone does this so everyone ends up with
  // the same model)
  svtkVariantArray* row4 = svtkVariantArray::New();
  row4->SetNumberOfValues(4);

  std::vector<svtkStdString>::iterator xyit = xyValues_l.begin();
  std::vector<svtkIdType>::iterator kcit = kcValues_l.begin();

  // First replace existing rows
  // Start with row 1 and not 0 because of cardinality row (cf. superclass for a detailed
  // explanation)
  for (svtkIdType r = 1; r < nRowCont; ++r, xyit += 2, kcit += 2)
  {
    row4->SetValue(0, *kcit);
    row4->SetValue(1, *xyit);
    row4->SetValue(2, *(xyit + 1));
    row4->SetValue(3, *(kcit + 1));

    contingencyTab->SetRow(r, row4);
  }

  // Then insert new rows
  for (; xyit != xyValues_l.end(); xyit += 2, kcit += 2)
  {
    row4->SetValue(0, *kcit);
    row4->SetValue(1, *xyit);
    row4->SetValue(2, *(xyit + 1));
    row4->SetValue(3, *(kcit + 1));

    contingencyTab->InsertNextRow(row4);
  }

  // Clean up
  row4->Delete();

  delete[] xyPacked_g;
  delete[] kcValues_g;
  delete[] xySize_g;
  delete[] kcSize_g;
  delete[] xyOffset;
  delete[] kcOffset;

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->StopTimer();

  cout << "## Process " << myRank << " parallel Learn took " << timer->GetElapsedTime()
       << " seconds."
       << "\n";

  timer->Delete();
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS
}

// ----------------------------------------------------------------------
bool svtkPContingencyStatistics::Reduce(svtkIdType& xySizeTotal, char* xyPacked_g,
  svtkStdString& xyPacked_l, svtkIdType& kcSizeTotal, svtkIdType* kcValues_g,
  std::vector<svtkIdType>& kcValues_l)
{
  // First, unpack the packet of strings
  std::vector<svtkStdString> xyValues_g;
  StringBufferToStringVector(svtkStdString(xyPacked_g, xySizeTotal), xyValues_g);

  // Second, check consistency: we must have the same number of xy and kc entries
  if (svtkIdType(xyValues_g.size()) != kcSizeTotal)
  {
    svtkErrorMacro("Reduction error on process "
      << this->Controller->GetCommunicator()->GetLocalProcessId()
      << ": inconsistent number of (x,y) and (k,c) pairs: " << xyValues_g.size() << " <> "
      << kcSizeTotal << ".");

    return true;
  }

  // Third, reduce to the global contingency table
  typedef std::map<svtkStdString, svtkIdType> Distribution;
  typedef std::map<svtkStdString, Distribution> Bidistribution;
  std::map<svtkIdType, Bidistribution> contingencyTable;
  svtkIdType i = 0;
  for (std::vector<svtkStdString>::iterator vit = xyValues_g.begin(); vit != xyValues_g.end();
       vit += 2, i += 2)
  {
    contingencyTable[kcValues_g[i]][*vit][*(vit + 1)] += kcValues_g[i + 1];
  }

  // Fourth, prepare send buffers of (global) xy and kc values
  std::vector<svtkStdString> xyValues_l;
  kcValues_l.clear();
  for (std::map<svtkIdType, Bidistribution>::iterator ait = contingencyTable.begin();
       ait != contingencyTable.end(); ++ait)
  {
    Bidistribution bidi = ait->second;
    for (Bidistribution::iterator bit = bidi.begin(); bit != bidi.end(); ++bit)
    {
      Distribution di = bit->second;
      for (Distribution::iterator dit = di.begin(); dit != di.end(); ++dit)
      {
        // Push back x and y to list of strings
        xyValues_l.push_back(bit->first); // x
        xyValues_l.push_back(dit->first); // y

        // Push back (X,Y) index and #(x,y) to list of strings
        kcValues_l.push_back(ait->first);  // k
        kcValues_l.push_back(dit->second); // c
      }
    }
  }
  StringVectorToStringBuffer(xyValues_l, xyPacked_l);

  // Last, update xy and kc buffer sizes (which have changed because of the reduction)
  xySizeTotal = static_cast<svtkIdType>(xyPacked_l.size());
  kcSizeTotal = static_cast<svtkIdType>(kcValues_l.size());

  return false;
}

// ----------------------------------------------------------------------
bool svtkPContingencyStatistics::Broadcast(svtkIdType xySizeTotal, svtkStdString& xyPacked,
  std::vector<svtkStdString>& xyValues, svtkIdType kcSizeTotal, std::vector<svtkIdType>& kcValues,
  svtkIdType rProc)
{
  svtkCommunicator* com = this->Controller->GetCommunicator();

  // Broadcast the xy and kc buffer sizes
  if (!com->Broadcast(&xySizeTotal, 1, rProc))
  {
    svtkErrorMacro(
      "Process " << com->GetLocalProcessId() << " could not broadcast (x,y) buffer size.");

    return true;
  }

  if (!com->Broadcast(&kcSizeTotal, 1, rProc))
  {
    svtkErrorMacro(
      "Process " << com->GetLocalProcessId() << " could not broadcast (k,c) buffer size.");

    return true;
  }

  // Resize vectors so they can receive the broadcasted xy and kc values
  xyPacked.resize(xySizeTotal);
  kcValues.resize(kcSizeTotal);

  // Broadcast the contents of contingency table to everyone
  if (!com->Broadcast(&(*xyPacked.begin()), xySizeTotal, rProc))
  {
    svtkErrorMacro("Process " << com->GetLocalProcessId() << " could not broadcast (x,y) values.");

    return true;
  }

  if (!com->Broadcast(&(*kcValues.begin()), kcSizeTotal, rProc))
  {
    svtkErrorMacro("Process " << com->GetLocalProcessId() << " could not broadcast (k,c) values.");

    return true;
  }

  // Unpack the packet of strings
  StringBufferToStringVector(xyPacked, xyValues);

  return false;
}
