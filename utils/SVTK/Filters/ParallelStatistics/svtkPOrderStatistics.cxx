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

#include "svtkPOrderStatistics.h"
#include "svtkToolkits.h"

#include "svtkCommunicator.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <map>
#include <set>
#include <vector>

svtkStandardNewMacro(svtkPOrderStatistics);
svtkCxxSetObjectMacro(svtkPOrderStatistics, Controller, svtkMultiProcessController);
//-----------------------------------------------------------------------------
svtkPOrderStatistics::svtkPOrderStatistics()
{
  this->Controller = 0;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
svtkPOrderStatistics::~svtkPOrderStatistics()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void svtkPOrderStatistics::PrintSelf(ostream& os, svtkIndent indent)
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
static void StringArrayToStringBuffer(svtkStringArray* sVals, svtkStdString& sPack)
{
  std::vector<svtkStdString> sVect; // consecutive strings

  svtkIdType nv = sVals->GetNumberOfValues();
  for (svtkIdType i = 0; i < nv; ++i)
  {
    // Push back current string value
    sVect.push_back(sVals->GetValue(i));
  }

  // Concatenate vector of strings into single string
  StringVectorToStringBuffer(sVect, sPack);
}

//-----------------------------------------------------------------------------
static void StringHistoToBuffers(
  const std::map<svtkStdString, svtkIdType>& histo, svtkStdString& buffer, svtkIdTypeArray* card)
{
  buffer.clear();

  card->SetNumberOfTuples(static_cast<svtkIdType>(histo.size()));

  svtkIdType r = 0;
  for (std::map<svtkStdString, svtkIdType>::const_iterator it = histo.begin(); it != histo.end();
       ++it, ++r)
  {
    buffer.append(it->first);
    card->SetValue(r, it->second);
    buffer.push_back(0);
  }
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
void svtkPOrderStatistics::Learn(
  svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  // First calculate order statistics on local data set
  this->Superclass::Learn(inData, inParameters, outMeta);

  if (!outMeta || outMeta->GetNumberOfBlocks() < 1)
  {
    // No statistics were calculated.
    return;
  }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
    return;
  }

  // Get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkErrorMacro("No parallel communicator.");
  }

  // Figure local process id
  svtkIdType myRank = com->GetLocalProcessId();

  // NB: Use process 0 as sole reducer for now
  svtkIdType rProc = 0;

  // Iterate over primary tables
  unsigned int nBlocks = outMeta->GetNumberOfBlocks();
  for (unsigned int b = 0; b < nBlocks; ++b)
  {
    // Fetch histogram table
    svtkTable* histoTab = svtkTable::SafeDownCast(outMeta->GetBlock(b));
    if (!histoTab)
    {
      continue;
    }

    // Downcast columns to typed arrays for efficient data access
    svtkAbstractArray* vals = histoTab->GetColumnByName("Value");
    svtkIdTypeArray* card =
      svtkArrayDownCast<svtkIdTypeArray>(histoTab->GetColumnByName("Cardinality"));
    if (!vals || !card)
    {
      svtkErrorMacro("Column fetching error on process " << myRank << ".");

      return;
    }

    // Create new table for global histogram
    svtkTable* histoTab_g = svtkTable::New();

    // Create column for global histogram cardinalities
    svtkIdTypeArray* card_g = svtkIdTypeArray::New();
    card_g->SetName("Cardinality");

    // Gather all histogram cardinalities on process rProc
    // NB: GatherV because the arrays have variable lengths
    if (!com->GatherV(card, card_g, rProc))
    {
      svtkErrorMacro(
        "Process " << com->GetLocalProcessId() << " could not gather histogram cardinalities.");

      return;
    }

    // Gather all histogram values on rProc and perform reduction of the global histogram table
    if (vals->IsA("svtkDataArray"))
    {
      // Downcast column to data array for subsequent typed message passing
      svtkDataArray* dVals = svtkArrayDownCast<svtkDataArray>(vals);

      // Create column for global histogram values of the same type as the values
      svtkDataArray* dVals_g = svtkDataArray::CreateDataArray(dVals->GetDataType());
      dVals_g->SetName("Value");

      // Gather all histogram values on process rProc
      // NB: GatherV because the arrays have variable lengths
      if (!com->GatherV(dVals, dVals_g, rProc))
      {
        svtkErrorMacro(
          "Process " << com->GetLocalProcessId() << " could not gather histogram values.");

        return;
      }

      // Reduce to global histogram table on process rProc
      if (myRank == rProc)
      {
        if (this->Reduce(card_g, dVals_g))
        {
          return;
        }
      } // if ( myRank == rProc )

      // Finally broadcast reduced histogram values
      if (!com->Broadcast(dVals_g, rProc))
      {
        svtkErrorMacro("Process " << com->GetLocalProcessId()
                                 << " could not broadcast reduced histogram values.");

        return;
      }

      // Add column of data values to histogram table
      histoTab_g->AddColumn(dVals_g);

      // Clean up
      dVals_g->Delete();

      // Finally broadcast reduced histogram cardinalities
      if (!com->Broadcast(card_g, rProc))
      {
        svtkErrorMacro("Process " << com->GetLocalProcessId()
                                 << " could not broadcast reduced histogram cardinalities.");

        return;
      }
    } // if ( vals->IsA("svtkDataArray") )
    else if (vals->IsA("svtkStringArray"))
    {
      // Downcast column to string array for subsequent typed message passing
      svtkStringArray* sVals = svtkArrayDownCast<svtkStringArray>(vals);

      // Packing step: concatenate all string values
      svtkStdString sPack_l;
      StringArrayToStringBuffer(sVals, sPack_l);

      // (All) gather all string sizes
      svtkIdType nc_l = static_cast<svtkIdType>(sPack_l.size());
      svtkIdType* nc_g = new svtkIdType[np];
      com->AllGather(&nc_l, nc_g, 1);

      // Calculate total size and displacement arrays
      svtkIdType* offsets = new svtkIdType[np];
      svtkIdType ncTotal = 0;

      for (svtkIdType i = 0; i < np; ++i)
      {
        offsets[i] = ncTotal;
        ncTotal += nc_g[i];
      }

      // Allocate receive buffer on reducer process, based on the global size obtained above
      char* sPack_g = 0;
      if (myRank == rProc)
      {
        sPack_g = new char[ncTotal];
      }

      // Gather all sPack on process rProc
      // NB: GatherV because the packets have variable lengths
      if (!com->GatherV(&(*sPack_l.begin()), sPack_g, nc_l, nc_g, offsets, rProc))
      {
        svtkErrorMacro("Process " << myRank << "could not gather string values.");

        return;
      }

      // Reduce to global histogram on process rProc
      std::map<svtkStdString, svtkIdType> histogram;
      if (myRank == rProc)
      {
        if (this->Reduce(card_g, ncTotal, sPack_g, histogram))
        {
          return;
        }
      } // if ( myRank == rProc )

      // Create column for global histogram values of the same type as the values
      svtkStringArray* sVals_g = svtkStringArray::New();
      sVals_g->SetName("Value");

      // Finally broadcast reduced histogram
      if (this->Broadcast(histogram, card_g, sVals_g, rProc))
      {
        svtkErrorMacro("Process " << com->GetLocalProcessId()
                                 << " could not broadcast reduced histogram values.");

        return;
      }

      // Add column of string values to histogram table
      histoTab_g->AddColumn(sVals_g);

      // Clean up
      sVals_g->Delete();
    } // else if ( vals->IsA("svtkStringArray") )
    else if (vals->IsA("svtkVariantArray"))
    {
      svtkErrorMacro(
        "Unsupported data type (variant array) for column " << vals->GetName() << ". Ignoring it.");
      return;
    }
    else
    {
      svtkErrorMacro("Unsupported data type for column " << vals->GetName() << ". Ignoring it.");
      return;
    }

    // Add column of cardinalities to histogram table
    histoTab_g->AddColumn(card_g);

    // Replace local histogram table with globally reduced one
    outMeta->SetBlock(b, histoTab_g);

    // Clean up
    card_g->Delete();
    histoTab_g->Delete();
  } // for ( unsigned int b = 0; b < nBlocks; ++ b )
}

//-----------------------------------------------------------------------------
bool svtkPOrderStatistics::Reduce(svtkIdTypeArray* card_g, svtkDataArray* dVals_g)
{
  // Check consistency: we must have as many values as cardinality entries
  svtkIdType nRow_g = card_g->GetNumberOfTuples();
  if (dVals_g->GetNumberOfTuples() != nRow_g)
  {
    svtkErrorMacro("Gathering error on process "
      << this->Controller->GetCommunicator()->GetLocalProcessId()
      << ": inconsistent number of values and cardinality entries: " << dVals_g->GetNumberOfTuples()
      << " <> " << nRow_g << ".");

    return true;
  }

  // Reduce to the global histogram table
  std::map<double, svtkIdType> histogram;
  double x;
  svtkIdType c;
  for (svtkIdType r = 0; r < nRow_g; ++r)
  {
    // First, fetch value
    x = dVals_g->GetTuple1(r);

    // Then, retrieve corresponding cardinality
    c = card_g->GetValue(r);

    // Last, update histogram count for corresponding value
    histogram[x] += c;
  }

  // Now resize global histogram arrays to reduced size
  nRow_g = static_cast<svtkIdType>(histogram.size());
  dVals_g->SetNumberOfTuples(nRow_g);
  card_g->SetNumberOfTuples(nRow_g);

  // Then store reduced histogram into array
  std::map<double, svtkIdType>::iterator hit = histogram.begin();
  for (svtkIdType r = 0; r < nRow_g; ++r, ++hit)
  {
    dVals_g->SetTuple1(r, hit->first);
    card_g->SetValue(r, hit->second);
  }

  return false;
}

//-----------------------------------------------------------------------------
bool svtkPOrderStatistics::Reduce(svtkIdTypeArray* card_g, svtkIdType& ncTotal, char* sPack_g,
  std::map<svtkStdString, svtkIdType>& histogram)
{
  // First, unpack the packet of strings
  std::vector<svtkStdString> sVect_g;
  StringBufferToStringVector(svtkStdString(sPack_g, ncTotal), sVect_g);

  // Second, check consistency: we must have as many values as cardinality entries
  svtkIdType nRow_g = card_g->GetNumberOfTuples();
  if (svtkIdType(sVect_g.size()) != nRow_g)
  {
    svtkErrorMacro("Gathering error on process "
      << this->Controller->GetCommunicator()->GetLocalProcessId()
      << ": inconsistent number of values and cardinality entries: " << sVect_g.size() << " <> "
      << nRow_g << ".");

    return true;
  }

  // Third, reduce to the global histogram
  svtkIdType c;
  svtkIdType i = 0;
  for (std::vector<svtkStdString>::iterator vit = sVect_g.begin(); vit != sVect_g.end(); ++vit, ++i)
  {
    // First, retrieve cardinality
    c = card_g->GetValue(i);

    // Then, update histogram count for corresponding value
    histogram[*vit] += c;
  }

  return false;
}

// ----------------------------------------------------------------------
bool svtkPOrderStatistics::Broadcast(std::map<svtkStdString, svtkIdType>& histogram,
  svtkIdTypeArray* card, svtkStringArray* sVals, svtkIdType rProc)
{
  svtkCommunicator* com = this->Controller->GetCommunicator();

  // Concatenate string keys of histogram into single string and put values into resized array
  svtkStdString sPack;
  StringHistoToBuffers(histogram, sPack, card);

  // Broadcast size of string buffer
  svtkIdType nc = static_cast<svtkIdType>(sPack.size());
  if (!com->Broadcast(&nc, 1, rProc))
  {
    svtkErrorMacro(
      "Process " << com->GetLocalProcessId() << " could not broadcast size of string buffer.");

    return true;
  }

  // Resize string so it can receive the broadcasted string buffer
  sPack.resize(nc);

  // Broadcast histogram values
  if (!com->Broadcast(&(*sPack.begin()), nc, rProc))
  {
    svtkErrorMacro(
      "Process " << com->GetLocalProcessId() << " could not broadcast histogram string values.");

    return true;
  }

  // Unpack the packet of strings
  std::vector<svtkStdString> sVect;
  StringBufferToStringVector(sPack, sVect);

  // Broadcast histogram cardinalities
  if (!com->Broadcast(card, rProc))
  {
    svtkErrorMacro(
      "Process " << com->GetLocalProcessId() << " could not broadcast histogram cardinalities.");

    return true;
  }

  // Now resize global histogram arrays to reduced size
  sVals->SetNumberOfValues(static_cast<svtkIdType>(sVect.size()));

  // Then store reduced histogram into array
  svtkIdType r = 0;
  for (std::vector<svtkStdString>::iterator vit = sVect.begin(); vit != sVect.end(); ++vit, ++r)
  {
    sVals->SetValue(r, *vit);
  }

  return false;
}
