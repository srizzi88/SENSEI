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
// This tests reading of a simple ADIOS2 bp file.

#include "svtkADIOS2CoreImageReader.h"

#include "svtkActor.h"
#include "svtkAlgorithm.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeRenderManager.h"
#include "svtkDataArray.h"
#include "svtkDataSetMapper.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiBlockVolumeMapper.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedIntArray.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLPMultiBlockDataWriter.h"

#include "svtkNew.h"
#include "svtkTestUtilities.h"

#include <sstream> // istringstream

struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};

void TestADIOS2BPReaderMPIMultiTimeSteps3D(svtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;

  svtkNew<svtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/3D_7-point_24-step/gs.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->SetController(controller);

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 3);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  // Enable multi time stesp
  reader->SetTimeStepArray("step");
  reader->SetDimensionArray("U");
  reader->SetArrayStatus("step", false);

  reader->SetActiveScalar(std::make_pair("U", svtkADIOS2CoreImageReader::VarType::CellData));
  reader->Update();

  svtkSmartPointer<svtkMultiBlockDataSet> output =
    svtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  svtkSmartPointer<svtkMultiPieceDataSet> mpds =
    svtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 6);
  svtkSmartPointer<svtkImageData> image0 = svtkImageData::SafeDownCast(mpds->GetPiece(0));
  svtkSmartPointer<svtkImageData> image1 = svtkImageData::SafeDownCast(mpds->GetPiece(1));

  // Use svtkXMLPMultiBlockDataWriter + svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()
  // to write out the data if needed

  *(args->retval) = 0;
  controller->Broadcast(args->retval, 1, 0);
}

int TestADIOS2BPReaderMPIMultiTimeSteps3D(int argc, char* argv[])
{
  int retval{ 0 };

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  svtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(TestADIOS2BPReaderMPIMultiTimeSteps3D, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
