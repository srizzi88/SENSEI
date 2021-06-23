/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUnstructuredGridWriter.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkCellIterator.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

#include <algorithm>
#include <iterator>
#include <vector>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkUnstructuredGridWriter);

void svtkUnstructuredGridWriter::WriteData()
{
  ostream* fp;
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::SafeDownCast(this->GetInput());
  int *types, ncells, cellId;

  svtkDebugMacro(<< "Writing svtk unstructured grid data...");

  if (!(fp = this->OpenSVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseSVTKFile(fp);
      unlink(this->FileName);
    }
    return;
  }
  //
  // Write unstructured grid specific stuff
  //
  *fp << "DATASET UNSTRUCTURED_GRID\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (!this->WritePoints(fp, input->GetPoints()))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  // Write cells. Check for faces so that we can handle them if present:
  if (input->GetFaces() != nullptr)
  {
    // Handle face data:
    if (!this->WriteCellsAndFaces(fp, input, "CELLS"))
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseSVTKFile(fp);
      unlink(this->FileName);
      return;
    }
  }
  else
  {
    // Fall back to superclass:
    if (!this->WriteCells(fp, input->GetCells(), "CELLS"))
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseSVTKFile(fp);
      unlink(this->FileName);
      return;
    }
  }

  //
  // Cell types are a little more work
  //
  if (input->GetCells())
  {
    ncells = input->GetCells()->GetNumberOfCells();
    types = new int[ncells];
    for (cellId = 0; cellId < ncells; cellId++)
    {
      types[cellId] = input->GetCellType(cellId);
    }

    *fp << "CELL_TYPES " << ncells << "\n";
    if (this->FileType == SVTK_ASCII)
    {
      for (cellId = 0; cellId < ncells; cellId++)
      {
        *fp << types[cellId] << "\n";
      }
    }
    else
    {
      // swap the bytes if necessary
      svtkByteSwap::SwapWrite4BERange(types, ncells, fp);
    }
    *fp << "\n";
    delete[] types;
  }

  if (!this->WriteCellData(fp, input))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WritePointData(fp, input))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  this->CloseSVTKFile(fp);
}

int svtkUnstructuredGridWriter::WriteCellsAndFaces(
  ostream* fp, svtkUnstructuredGrid* grid, const char* label)
{
  if (!grid->GetCells())
  {
    return 1;
  }

  // Create a copy of the cell data with the face streams expanded.
  // Do this before writing anything so that we know the size.
  // Use ints to represent svtkIdTypes, since that's what the superclass does.
  svtkNew<svtkCellArray> expandedCells;
  expandedCells->AllocateEstimate(grid->GetNumberOfCells(), grid->GetMaxCellSize());

  svtkSmartPointer<svtkCellIterator> it =
    svtkSmartPointer<svtkCellIterator>::Take(grid->NewCellIterator());

  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    if (it->GetCellType() != SVTK_POLYHEDRON)
    {
      expandedCells->InsertNextCell(it->GetPointIds());
    }
    else
    {
      expandedCells->InsertNextCell(it->GetFaces());
    }
  }

  if (expandedCells->GetNumberOfCells() == 0)
  { // Nothing to do.
    return 1;
  }

  if (!this->WriteCells(fp, expandedCells, label))
  {
    svtkErrorMacro("Error while writing expanded face stream.");
    return 0;
  }

  fp->flush();
  if (fp->fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }

  return 1;
}

int svtkUnstructuredGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  return 1;
}

svtkUnstructuredGrid* svtkUnstructuredGridWriter::GetInput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput());
}

svtkUnstructuredGrid* svtkUnstructuredGridWriter::GetInput(int port)
{
  return svtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkUnstructuredGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
