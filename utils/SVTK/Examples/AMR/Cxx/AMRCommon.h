/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRCommon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRCommon.h -- Encapsulates common functionality for AMR data.
//
// .SECTION Description
// This header encapsulates some common functionality for AMR data to
// simplify and expedite the development of examples.

#ifndef AMRCOMMON_H_
#define AMRCOMMON_H_

#include <cassert> // For C++ assert
#include <sstream> // For C++ string streams

#include "svtkCell.h"
#include "svtkCompositeDataWriter.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkImageToStructuredGrid.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkOverlappingAMR.h"
#include "svtkStructuredGridWriter.h"
#include "svtkUniformGrid.h"
#include "svtkXMLHierarchicalBoxDataReader.h"
#include "svtkXMLImageDataWriter.h"
#include "svtkXMLMultiBlockDataWriter.h"
#include "svtkXMLUniformGridAMRWriter.h"

namespace AMRCommon
{

//------------------------------------------------------------------------------
// Description:
// Writes a uniform grid as a structure grid
void WriteUniformGrid(svtkUniformGrid* g, const std::string& prefix)
{
  assert("pre: Uniform grid (g) is NULL!" && (g != nullptr));

  svtkXMLImageDataWriter* imgWriter = svtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << imgWriter->GetDefaultFileExtension();
  imgWriter->SetFileName(oss.str().c_str());
  imgWriter->SetInputData(g);
  imgWriter->Write();

  imgWriter->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Writes the given AMR dataset to a *.vth file with the given prefix.
void WriteAMRData(svtkOverlappingAMR* amrData, const std::string& prefix)
{
  // Sanity check
  assert("pre: AMR dataset is NULL!" && (amrData != nullptr));

  svtkCompositeDataWriter* writer = svtkCompositeDataWriter::New();

  std::ostringstream oss;
  oss << prefix << ".vthb";
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(amrData);
  writer->Write();
  writer->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Reads AMR data to the given data-structure from the prescribed file.
svtkHierarchicalBoxDataSet* ReadAMRData(const std::string& file)
{
  // Sanity check
  //  assert( "pre: AMR dataset is NULL!" && (amrData != NULL) );

  svtkXMLHierarchicalBoxDataReader* myAMRReader = svtkXMLHierarchicalBoxDataReader::New();
  assert("pre: AMR Reader is NULL!" && (myAMRReader != nullptr));

  std::ostringstream oss;
  oss.str("");
  oss.clear();
  oss << file << ".vthb";

  std::cout << "Reading AMR Data from: " << oss.str() << std::endl;
  std::cout.flush();

  myAMRReader->SetFileName(oss.str().c_str());
  myAMRReader->Update();

  svtkHierarchicalBoxDataSet* amrData =
    svtkHierarchicalBoxDataSet::SafeDownCast(myAMRReader->GetOutput());
  assert("post: AMR data read is NULL!" && (amrData != nullptr));
  return (amrData);
}

//------------------------------------------------------------------------------
// Description:
// Writes the given multi-block data to an XML file with the prescribed prefix
void WriteMultiBlockData(svtkMultiBlockDataSet* mbds, const std::string& prefix)
{
  // Sanity check
  assert("pre: Multi-block dataset is NULL" && (mbds != nullptr));
  svtkXMLMultiBlockDataWriter* writer = svtkXMLMultiBlockDataWriter::New();

  std::ostringstream oss;
  oss.str("");
  oss.clear();
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(mbds);
  writer->Write();
  writer->Delete();
}

//------------------------------------------------------------------------------
// Constructs a uniform grid instance given the prescribed
// origin, grid spacing and dimensions.
svtkUniformGrid* GetGrid(double* origin, double* h, int* ndim)
{
  svtkUniformGrid* grd = svtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin(origin);
  grd->SetSpacing(h);
  grd->SetDimensions(ndim);
  return grd;
}

//------------------------------------------------------------------------------
// Computes the cell center for the cell corresponding to cellIdx w.r.t.
// the given grid. The cell center is stored in the supplied buffer c.
void ComputeCellCenter(svtkUniformGrid* grid, const int cellIdx, double c[3])
{
  assert("pre: grid != NULL" && (grid != nullptr));
  assert("pre: Null cell center buffer" && (c != nullptr));
  assert("pre: cellIdx in bounds" && (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()));

  svtkCell* myCell = grid->GetCell(cellIdx);
  assert("post: cell is NULL" && (myCell != nullptr));

  double pCenter[3];
  double* weights = new double[myCell->GetNumberOfPoints()];
  int subId = myCell->GetParametricCenter(pCenter);
  myCell->EvaluateLocation(subId, pCenter, c, weights);
  delete[] weights;
}

} // END namespace

#endif /* AMRCOMMON_H_ */
