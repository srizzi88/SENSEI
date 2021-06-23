/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPixelExtentIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPixelExtentIO.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkDataSetWriter.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkPoints.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

using std::deque;

// ----------------------------------------------------------------------------
svtkUnstructuredGrid& operator<<(svtkUnstructuredGrid& data, const svtkPixelExtent& ext)
{
  // initialize empty dataset
  if (data.GetNumberOfCells() < 1)
  {
    svtkPoints* opts = svtkPoints::New();
    data.SetPoints(opts);
    opts->Delete();

    svtkCellArray* cells = svtkCellArray::New();
    svtkUnsignedCharArray* types = svtkUnsignedCharArray::New();

    data.SetCells(types, cells);

    cells->Delete();
    types->Delete();
  }

  // cell to node
  svtkPixelExtent next(ext);
  next.CellToNode();

  // build the cell
  svtkFloatArray* pts = dynamic_cast<svtkFloatArray*>(data.GetPoints()->GetData());
  svtkIdType ptId = pts->GetNumberOfTuples();
  float* ppts = pts->WritePointer(3 * ptId, 12);

  int id[12] = { 0, 2, -1, 1, 2, -1, 1, 3, -1, 0, 3, -1 };

  svtkIdType ptIds[4];

  for (int i = 0; i < 4; ++i)
  {
    ppts[3 * i + 2] = 0.0;
    for (int j = 0; j < 2; ++j)
    {
      int q = 3 * i + j;
      ppts[q] = next[id[q]];
    }
    ptIds[i] = ptId + i;
  }

  data.InsertNextCell(SVTK_QUAD, 4, ptIds);

  return data;
}

// ----------------------------------------------------------------------------
void svtkPixelExtentIO::Write(
  int commRank, const char* fileName, const deque<deque<svtkPixelExtent> >& exts)
{
  if (commRank != 0)
  {
    // only rank 0 writes
    return;
  }

  svtkUnstructuredGrid* data = svtkUnstructuredGrid::New();

  svtkIntArray* rank = svtkIntArray::New();
  rank->SetName("rank");
  data->GetCellData()->AddArray(rank);
  rank->Delete();

  svtkIntArray* block = svtkIntArray::New();
  block->SetName("block");
  data->GetCellData()->AddArray(block);
  block->Delete();

  size_t nRanks = exts.size();

  for (size_t i = 0; i < nRanks; ++i)
  {
    size_t nBlocks = exts[i].size();
    for (size_t j = 0; j < nBlocks; ++j)
    {
      const svtkPixelExtent& ext = exts[i][j];
      *data << ext;

      rank->InsertNextTuple1(i);
      block->InsertNextTuple1(j);
    }
  }

  svtkDataSetWriter* idw = svtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}

// ----------------------------------------------------------------------------
void svtkPixelExtentIO::Write(int commRank, const char* fileName, const deque<svtkPixelExtent>& exts)
{
  if (commRank != 0)
  {
    // only rank 0 will write
    return;
  }

  svtkUnstructuredGrid* data = svtkUnstructuredGrid::New();

  svtkIntArray* rank = svtkIntArray::New();
  rank->SetName("rank");
  data->GetCellData()->AddArray(rank);
  rank->Delete();

  int nExts = static_cast<int>(exts.size());
  rank->SetNumberOfTuples(nExts);

  int* pRank = rank->GetPointer(0);

  for (int i = 0; i < nExts; ++i)
  {
    const svtkPixelExtent& ext = exts[i];
    *data << ext;
    pRank[i] = i;
  }

  svtkDataSetWriter* idw = svtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}

// ----------------------------------------------------------------------------
void svtkPixelExtentIO::Write(int commRank, const char* fileName, const svtkPixelExtent& ext)
{
  svtkUnstructuredGrid* data = svtkUnstructuredGrid::New();

  svtkIntArray* rank = svtkIntArray::New();
  rank->SetName("rank");
  data->GetCellData()->AddArray(rank);
  rank->Delete();

  rank->SetNumberOfTuples(1);
  int* pRank = rank->GetPointer(0);

  *data << ext;
  pRank[0] = commRank;

  svtkDataSetWriter* idw = svtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}
