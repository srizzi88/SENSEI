/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// SVTK includes
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkExtractRectilinearGrid.h"
#include "svtkMathUtilities.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPointDataToCellData.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridWriter.h"
#include "svtkStructuredData.h"

// C/C++ includes
#include <cassert>
#include <cmath>
#include <sstream>

//#define DEBUG

double exponential_distribution(const int i, const double beta)
{
  double xi = ((exp(i * beta) - 1) / (exp(beta) - 1));
  return (xi);
}

//------------------------------------------------------------------------------
void WriteGrid(svtkRectilinearGrid* grid, const std::string& file)
{
  assert("pre: input grid instance is nullptr!" && (grid != nullptr));

  std::ostringstream oss;
  oss << file << ".svtk";

  svtkRectilinearGridWriter* writer = svtkRectilinearGridWriter::New();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(grid);
  writer->Write();
  writer->Delete();
}

//------------------------------------------------------------------------------
int CheckGrid(svtkRectilinearGrid* grid)
{
  int rc = 0;

  svtkPointData* PD = grid->GetPointData();
  if (!PD->HasArray("xyz"))
  {
    ++rc;
    return (rc);
  }

  svtkDoubleArray* xyz_data = svtkArrayDownCast<svtkDoubleArray>(PD->GetArray("xyz"));
  double* xyz = static_cast<double*>(xyz_data->GetVoidPointer(0));

  svtkIdType npoints = grid->GetNumberOfPoints();
  for (svtkIdType pntIdx = 0; pntIdx < npoints; ++pntIdx)
  {
    double* pnt = grid->GetPoint(pntIdx);
    if (!svtkMathUtilities::NearlyEqual(pnt[0], xyz[pntIdx * 3], 1.e-9) ||
      !svtkMathUtilities::NearlyEqual(pnt[1], xyz[pntIdx * 3 + 1], 1.e-9) ||
      !svtkMathUtilities::NearlyEqual(pnt[2], xyz[pntIdx * 3 + 2], 1.e-9))
    {
      std::cerr << "ERROR: point=(" << pnt[0] << ", ";
      std::cerr << pnt[1] << ", ";
      std::cerr << pnt[2] << ") ";
      std::cerr << "data = (" << xyz[pntIdx * 3] << ", ";
      std::cerr << xyz[pntIdx * 3 + 1] << ", ";
      std::cerr << xyz[pntIdx * 3 + 2] << ") ";
      ++rc;
    } // END if
  }   // END for all points

  return (rc);
}

//------------------------------------------------------------------------------
void GenerateGrid(svtkRectilinearGrid* grid, int ext[6])
{
  assert("pre: input grid instance is nullptr!" && (grid != nullptr));
  grid->Initialize();
  grid->SetExtent(ext);

  svtkDataArray* coords[3];

  int dims[3];
  int dataDesc = svtkStructuredData::GetDataDescriptionFromExtent(ext);
  svtkStructuredData::GetDimensionsFromExtent(ext, dims, dataDesc);

  // compute & populate coordinate vectors
  double beta = 0.05; /* controls the intensity of the stretching */
  for (int i = 0; i < 3; ++i)
  {
    coords[i] = svtkDataArray::CreateDataArray(SVTK_DOUBLE);
    if (dims[i] == 0)
    {
      continue;
    }
    coords[i]->SetNumberOfTuples(dims[i]);

    double prev = 0.0;
    for (int j = 0; j < dims[i]; ++j)
    {
      double val = prev + ((j == 0) ? 0.0 : exponential_distribution(j, beta));
      coords[i]->SetTuple(j, &val);
      prev = val;
    } // END for all points along this dimension

  } // END for all dimensions

  grid->SetXCoordinates(coords[0]);
  grid->SetYCoordinates(coords[1]);
  grid->SetZCoordinates(coords[2]);
  coords[0]->Delete();
  coords[1]->Delete();
  coords[2]->Delete();

  // compute & populate XYZ field
  svtkIdType npoints = svtkStructuredData::GetNumberOfPoints(ext, dataDesc);
  svtkDoubleArray* xyz = svtkDoubleArray::New();
  xyz->SetName("xyz");
  xyz->SetNumberOfComponents(3);
  xyz->SetNumberOfTuples(npoints);

  for (svtkIdType pntIdx = 0; pntIdx < npoints; ++pntIdx)
  {
    xyz->SetTuple(pntIdx, grid->GetPoint(pntIdx));
  } // END for all points
  grid->GetPointData()->AddArray(xyz);

  svtkNew<svtkPointDataToCellData> pd2cd;
  pd2cd->PassPointDataOn();
  pd2cd->SetInputDataObject(grid);
  pd2cd->Update();
  grid->ShallowCopy(pd2cd->GetOutputDataObject(0));

  xyz->Delete();
}

//------------------------------------------------------------------------------
int TestExtractRectilinearGrid(int argc, char* argv[])
{
  int rc = 0;

  // silence compiler warnings
  static_cast<void>(argc);
  static_cast<void>(argv);

  int ext[6] = { 0, 49, 0, 49, 0, 0 };
  svtkRectilinearGrid* grid = svtkRectilinearGrid::New();
  GenerateGrid(grid, ext);
#ifdef DEBUG
  WriteGrid(grid, "initial");
#endif

  int sub_ext[6] = { 0, 35, 0, 35, 0, 0 };
  svtkExtractRectilinearGrid* extractFilter = svtkExtractRectilinearGrid::New();
  extractFilter->SetInputData(grid);
  extractFilter->SetVOI(sub_ext);
  extractFilter->SetSampleRate(2, 2, 1);
  extractFilter->IncludeBoundaryOn();
  extractFilter->Update();

  svtkRectilinearGrid* subGrid = extractFilter->GetOutput();
#ifdef DEBUG
  WriteGrid(subGrid, "sub-grid");
#endif

  rc += CheckGrid(subGrid);

  // Let's extract outer face too.
  int sub_ext2[6] = { 49, 49, 0, 49, 0, 0 };
  extractFilter->SetVOI(sub_ext2);
  extractFilter->SetSampleRate(1, 1, 1);
  extractFilter->IncludeBoundaryOff();
  extractFilter->Update();

  subGrid = extractFilter->GetOutput();
  rc += CheckGrid(subGrid);

  extractFilter->Delete();
  grid->Delete();
  return (rc);
}
