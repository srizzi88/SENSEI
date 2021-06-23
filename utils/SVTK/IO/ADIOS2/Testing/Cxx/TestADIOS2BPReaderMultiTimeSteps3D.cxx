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

int TestADIOS2BPReaderMultiTimeSteps3D(int argc, char* argv[])
{
  svtkNew<svtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/3D_7-point_24-step/gs.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return 0;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

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

  svtkNew<svtkImageDataToPointSet> imageToPointset;

  assert(image0->GetCellData()->GetNumberOfArrays() == 2);
  image0->GetCellData()->SetActiveScalars("U");
  imageToPointset->SetInputData(image0);

  imageToPointset->Update();

  // Since I fail to find a proper mapper to render two svtkImageDatas inside
  // a svtkMultiPieceDataSet in a svtkMultiBlockDataSet, I render the image directly here
  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputDataObject(imageToPointset->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("U", 0);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->GetActiveCamera()->Elevation(300);
  renderer->GetActiveCamera()->Yaw(60);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> rendWin;
  rendWin->SetSize(600, 300);
  rendWin->AddRenderer(renderer);

  rendWin->Render();

  // Do the test comparsion
  int retval = svtkRegressionTestImage(rendWin);
  if (retval == svtkRegressionTester::DO_INTERACTOR)
  {
    svtkNew<svtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(rendWin);
    iren->Initialize();
    iren->Start();
    retval = svtkRegressionTester::PASSED;
  }

  return !retval;
}
