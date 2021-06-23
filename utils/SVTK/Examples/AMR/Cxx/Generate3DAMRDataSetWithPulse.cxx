/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Generate3DAMRDataSetWithPulse.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Generate3DAMRDataSetWithPulse.cxx -- Generated sample 3D AMR dataset
//
// .SECTION Description
//  This utility code generates a simple 3D AMR dataset with a gaussian
//  pulse at the center. The resulting AMR dataset is written using the
//  svtkXMLHierarchicalBoxDataSetWriter.

// disable linking warning due to inclusion of svtkXML*
#if defined(_MSC_VER)
#pragma warning(disable : 4089)
#endif

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

#include "AMRCommon.h"
#include "svtkAMRBox.h"
#include "svtkAMRUtilities.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkOverlappingAMR.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkUniformGrid.h"

static struct PulseAttributes
{
  double origin[3]; // xyz for the center of the pulse
  double width[3];  // the width of the pulse
  double amplitude; // the amplitude of the pulse
} Pulse;

//
// Function prototype declarations
//

// Description:
// Sets the pulse attributes
void SetPulse();

// Description:
// Constructs the svtkHierarchicalBoxDataSet.
svtkOverlappingAMR* GetAMRDataSet();

// Description:
// Attaches the pulse to the given grid.
void AttachPulseToGrid(svtkUniformGrid* grid);

//
// Program main
//
int main(int argc, char** argv)
{
  // Fix compiler warning for unused variables
  static_cast<void>(argc);
  static_cast<void>(argv);

  // STEP 0: Initialize gaussian pulse parameters
  SetPulse();

  // STEP 1: Get the AMR dataset
  svtkOverlappingAMR* amrDataSet = GetAMRDataSet();
  assert("pre: nullptr AMR dataset" && (amrDataSet != nullptr));

  AMRCommon::WriteAMRData(amrDataSet, "Gaussian3D");
  amrDataSet->Delete();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

void SetPulse()
{
  Pulse.origin[0] = Pulse.origin[1] = Pulse.origin[2] = -1.0;
  Pulse.width[0] = Pulse.width[1] = Pulse.width[2] = 6.0;
  Pulse.amplitude = 0.0001;
}

//------------------------------------------------------------------------------
void AttachPulseToGrid(svtkUniformGrid* grid)
{
  assert("pre: grid is nullptr!" && (grid != nullptr));

  svtkDoubleArray* xyz = svtkDoubleArray::New();
  xyz->SetName("GaussianPulse");
  xyz->SetNumberOfComponents(1);
  xyz->SetNumberOfTuples(grid->GetNumberOfCells());

  for (int cellIdx = 0; cellIdx < grid->GetNumberOfCells(); ++cellIdx)
  {
    double center[3];
    AMRCommon::ComputeCellCenter(grid, cellIdx, center);

    double r = 0.0;
    for (int i = 0; i < 3; ++i)
    {
      double dx = center[i] - Pulse.origin[i];
      r += (dx * dx) / (Pulse.width[i] * Pulse.width[i]);
    }
    double f = Pulse.amplitude * std::exp(-r);

    xyz->SetTuple1(cellIdx, f);
  } // END for all cells

  grid->GetCellData()->AddArray(xyz);
  xyz->Delete();
}
//------------------------------------------------------------------------------
svtkOverlappingAMR* GetAMRDataSet()
{
  svtkOverlappingAMR* data = svtkOverlappingAMR::New();
  int blocksPerLevel[2] = { 1, 3 };
  double globalOrigin[3] = { -2.0, -2.0, -2.0 };
  data->Initialize(2, blocksPerLevel);
  data->SetOrigin(globalOrigin);
  data->SetGridDescription(SVTK_XYZ_GRID);

  double origin[3];
  double h[3];
  int ndim[3];

  // Root Block -- Block 0
  ndim[0] = 6;
  ndim[1] = ndim[2] = 5;
  h[0] = h[1] = h[2] = 1.0;
  origin[0] = origin[1] = origin[2] = -2.0;
  int blockId = 0;
  int level = 0;
  svtkUniformGrid* root = AMRCommon::GetGrid(origin, h, ndim);
  svtkAMRBox box(origin, ndim, h, data->GetOrigin(), data->GetGridDescription());
  AttachPulseToGrid(root);
  data->SetAMRBox(level, blockId, box);
  data->SetDataSet(level, blockId, root);
  root->Delete();

  // Block 1
  ndim[0] = 3;
  ndim[1] = ndim[2] = 5;
  h[0] = h[1] = h[2] = 0.5;
  origin[0] = origin[1] = origin[2] = -2.0;
  blockId = 0;
  level = 1;
  svtkUniformGrid* grid1 = AMRCommon::GetGrid(origin, h, ndim);
  AttachPulseToGrid(grid1);
  svtkAMRBox box1(origin, ndim, h, data->GetOrigin(), data->GetGridDescription());
  data->SetAMRBox(level, blockId, box1);
  data->SetDataSet(level, blockId, grid1);
  grid1->Delete();

  // Block 2
  ndim[0] = 3;
  ndim[1] = ndim[2] = 5;
  h[0] = h[1] = h[2] = 0.5;
  origin[0] = 0.0;
  origin[1] = origin[2] = -1.0;
  blockId = 1;
  level = 1;
  svtkUniformGrid* grid2 = AMRCommon::GetGrid(origin, h, ndim);
  AttachPulseToGrid(grid2);
  svtkAMRBox box2(origin, ndim, h, data->GetOrigin(), data->GetGridDescription());
  data->SetAMRBox(level, blockId, box2);
  data->SetDataSet(level, blockId, grid2);
  grid2->Delete();

  // Block 3
  ndim[0] = 3;
  ndim[1] = ndim[2] = 7;
  h[0] = h[1] = h[2] = 0.5;
  origin[0] = 2.0;
  origin[1] = origin[2] = -1.0;
  blockId = 2;
  level = 1;
  svtkUniformGrid* grid3 = AMRCommon::GetGrid(origin, h, ndim);
  svtkAMRBox box3(origin, ndim, h, data->GetOrigin(), data->GetGridDescription());
  AttachPulseToGrid(grid3);
  data->SetAMRBox(level, blockId, box3);
  data->SetDataSet(level, blockId, grid3);
  grid3->Delete();

  svtkAMRUtilities::BlankCells(data);
  return (data);
}
