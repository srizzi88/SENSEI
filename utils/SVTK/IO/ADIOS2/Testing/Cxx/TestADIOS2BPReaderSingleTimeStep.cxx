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
#include "svtkCompositePolyDataMapper.h"
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

#include "svtkNew.h"
#include "svtkTestUtilities.h"

#include <sstream> // istringstream

int TestADIOS2BPReaderSingleTimeStep(int argc, char* argv[])
{
  svtkNew<svtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/HeatMap3D/HeatMap3D.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return 0;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 2);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  reader->SetOrigin(0.0, 0.0, 0.0);
  reader->SetSpacing(1.0, 1.0, 1.0);
  reader->SetDimensionArray("temperature");
  reader->SetActiveScalar({ "temperature", svtkADIOS2CoreImageReader::VarType::CellData });

  reader->Update();

  svtkSmartPointer<svtkMultiBlockDataSet> output =
    svtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  svtkSmartPointer<svtkMultiPieceDataSet> mpds =
    svtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 2);
  svtkSmartPointer<svtkImageData> image0 = svtkImageData::SafeDownCast(mpds->GetPiece(0));

  // Use svtkXMLPMultiBlockDataWriter if you want to dump the data

  // Render the first image for testing
  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputDataObject(image0);
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("temperature", 0);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(2000);

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
