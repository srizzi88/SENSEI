/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalXdmfReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// This tests temporal reading and writing of static meshes using
// svtkXdmfReader and svtkXdmfWriter.

#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTestUtilities.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXdmfReader.h"
#include "svtkXdmfWriter.h"
#include "svtksys/SystemTools.hxx"

#define ASSERT_TEST(_cond_, _msg_)                                                                 \
  if (!(_cond_))                                                                                   \
  {                                                                                                \
    std::cerr << _msg_ << std::endl;                                                               \
    return SVTK_ERROR;                                                                              \
  }

int TestStaticMesh(svtkXdmfReader* reader)
{
  reader->UpdateInformation();

  svtkInformation* outInfo = reader->GetExecutive()->GetOutputInformation(0);

  int steps = (outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    ? outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS())
    : 0;
  ASSERT_TEST(steps == 3, "Read data does not have 3 time steps as expected!");
  double* timeSteps = outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

  svtkPoints* geometryAtT0 = 0;
  svtkCellArray* topologyAtT0 = 0;
  for (int i = 0; i < steps; i++)
  {
    double updateTime = timeSteps[i];
    outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    reader->Update();
    svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    ASSERT_TEST(mb, "Root data is not a multiblock data set as expected!");
    ASSERT_TEST(mb->GetNumberOfBlocks() == 2, "Root multiblock data is supposed to have 2 blocks!");
    svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(mb->GetBlock(0));
    ASSERT_TEST(grid, "Block 0 is not an unstructured grid as expected!");
    if (i == 0)
    {
      geometryAtT0 = grid->GetPoints();
      topologyAtT0 = grid->GetCells();
    }

    ASSERT_TEST(grid->GetPoints() == geometryAtT0, "Geometry is not static over time as expected!");
    ASSERT_TEST(grid->GetCells() == topologyAtT0, "Topology is not static over time as expected!");
  }
  return 0;
}

int TestTemporalXdmfReaderWriter(int argc, char* argv[])
{
  // Read the input data file
  char* filePath =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/XDMF/temporalStaticMeshes.xmf");
  svtkNew<svtkXdmfReader> reader;
  reader->SetFileName(filePath);
  delete[] filePath;
  if (TestStaticMesh(reader) == SVTK_ERROR)
  {
    std::cerr << "Error while reading " << reader->GetFileName() << std::endl;
    return SVTK_ERROR;
  }

  // Write the input data to a new Xdmf file
  std::string outFilePath = "temporalStaticMeshesTest.xmf";
  svtkNew<svtkXdmfWriter> writer;
  writer->SetFileName(outFilePath.c_str());
  writer->WriteAllTimeStepsOn();
  writer->MeshStaticOverTimeOn();
  writer->SetInputConnection(reader->GetOutputPort());
  writer->Write();

  // Test written file
  svtkNew<svtkXdmfReader> reader2;
  reader2->SetFileName(outFilePath.c_str());
  if (TestStaticMesh(reader2) == SVTK_ERROR)
  {
    std::cerr << "Error while reading " << reader2->GetFileName() << std::endl;
    return SVTK_ERROR;
  }

  return 0;
}
