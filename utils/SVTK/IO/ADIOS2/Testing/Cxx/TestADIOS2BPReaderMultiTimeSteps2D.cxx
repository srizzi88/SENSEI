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
#include "svtkCompositeRenderManager.h"
#include "svtkDataArray.h"
#include "svtkDataSetMapper.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnsignedIntArray.h"
#include "svtkXMLPMultiBlockDataWriter.h"

#include "svtkNew.h"
#include "svtkTestUtilities.h"

#include <sstream> // istringstream

int TestADIOS2BPReaderMultiTimeSteps2D(int argc, char* argv[])
{

  svtkNew<svtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/2D_HeatTransfer_10-step/sim.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return 0;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 1);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  reader->SetDimensionArray("T");
  reader->SetTimeStepArray("T");

  reader->Update();

  svtkSmartPointer<svtkMultiBlockDataSet> output =
    svtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  svtkSmartPointer<svtkMultiPieceDataSet> mpds =
    svtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 2);
  svtkSmartPointer<svtkImageData> image0 = svtkImageData::SafeDownCast(mpds->GetPiece(0));
  svtkSmartPointer<svtkImageData> image1 = svtkImageData::SafeDownCast(mpds->GetPiece(1));
  assert(image0);
  assert(image1);

  svtkNew<svtkImageDataToPointSet> imageToPointset;

  assert(image0->GetCellData()->GetNumberOfArrays() == 1);
  image0->GetCellData()->SetActiveScalars("T");
  imageToPointset->SetInputData(image0);

  imageToPointset->Update();

  // Since I fail to find a proper mapper to render two svtkImageDatas inside
  // a svtkMultiPieceDataSet in a svtkMultiBlockDataSet, I render the image directly here
  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputDataObject(imageToPointset->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("T", 0);

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
