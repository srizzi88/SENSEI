/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSplitByCellScalarFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "svtkCellData.h"
#include "svtkDataSetAttributes.h"
#include "svtkDataSetTriangleFilter.h"
#include "svtkGeometryFilter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkSplitByCellScalarFilter.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLImageDataReader.h"

int TestSplitByCellScalarFilter(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/waveletMaterial.vti");

  svtkNew<svtkXMLImageDataReader> reader;
  reader->SetFileName(fname);
  if (!reader->CanReadFile(fname))
  {
    std::cerr << "Error: Could not read " << fname << ".\n";
    delete[] fname;
    return EXIT_FAILURE;
  }
  reader->Update();
  delete[] fname;

  svtkImageData* image = reader->GetOutput();

  double range[2];
  image->GetCellData()->GetScalars()->GetRange(range);
  unsigned int nbMaterials = range[1] - range[0] + 1;

  // Test with image data input
  svtkNew<svtkSplitByCellScalarFilter> split;
  split->SetInputData(image);
  split->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);
  split->Update();

  svtkMultiBlockDataSet* output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() << " blocks instead of "
              << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int cc = 0; cc < nbMaterials; ++cc)
  {
    auto name = output->GetMetaData(cc)->Get(svtkCompositeDataSet::NAME());
    auto oscalars = svtkDataSet::SafeDownCast(output->GetBlock(cc))->GetCellData()->GetScalars();
    double r[2];
    oscalars->GetRange(r);
    auto blockname = std::string("Material_") + std::to_string(static_cast<int>(r[0]));
    if (name == nullptr || blockname != name)
    {
      cerr << "Mismatched block names" << endl;
      return EXIT_FAILURE;
    }
  }

  // Test with unstructured grid input and pass all points option turned on
  svtkNew<svtkDataSetTriangleFilter> triangulate;
  triangulate->SetInputData(image);
  triangulate->Update();

  svtkUnstructuredGrid* grid = triangulate->GetOutput();
  split->SetInputData(grid);
  split->PassAllPointsOn();
  split->Update();

  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() << " blocks instead of "
              << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    svtkUnstructuredGrid* ug = svtkUnstructuredGrid::SafeDownCast(output->GetBlock(i));
    if (!ug || ug->GetNumberOfPoints() != grid->GetNumberOfPoints())
    {
      std::cerr << "Output grid " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with unstructured grid input and pass all points option turned off
  split->PassAllPointsOff();
  split->Update();
  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() << " blocks instead of "
              << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    svtkUnstructuredGrid* ug = svtkUnstructuredGrid::SafeDownCast(output->GetBlock(i));
    if (!ug || ug->GetNumberOfPoints() == grid->GetNumberOfPoints())
    {
      std::cerr << "Output grid " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with polydata input and pass all points option turned on
  svtkNew<svtkGeometryFilter> geom;
  geom->SetInputData(grid);
  geom->Update();

  svtkPolyData* mesh = geom->GetOutput();
  split->SetInputData(mesh);
  split->PassAllPointsOn();
  split->Update();
  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() << " blocks instead of "
              << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    svtkPolyData* outMesh = svtkPolyData::SafeDownCast(output->GetBlock(i));
    if (!outMesh || outMesh->GetNumberOfPoints() != grid->GetNumberOfPoints())
    {
      std::cerr << "Output mesh " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with polydata input and pass all points option turned off
  split->PassAllPointsOff();
  split->Update();
  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() << " blocks instead of "
              << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    svtkPolyData* outMesh = svtkPolyData::SafeDownCast(output->GetBlock(i));
    if (!outMesh || outMesh->GetNumberOfPoints() == grid->GetNumberOfPoints())
    {
      std::cerr << "Output mesh " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
