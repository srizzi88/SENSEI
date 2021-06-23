/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReaderTextureUV.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkPLYReader
// .SECTION Description
//

#include "svtkPLYReader.h"

#include "svtkActor.h"
#include "svtkPNGReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"

int TestPLYReaderTextureUV(int argc, char* argv[])
{
  // Read file name.
  if (argc < 2)
  {
    return EXIT_FAILURE;
  }
  std::string fn = "Data/";
  std::string plyName = fn + argv[1];
  std::string imageName = fn + argv[2];
  const char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, plyName.c_str());
  const char* fnameImg = svtkTestUtilities::ExpandDataFileName(argc, argv, imageName.c_str());

  // Test if the reader thinks it can open the file.
  if (0 == svtkPLYReader::CanReadFile(fname))
  {
    std::cout << "The PLY reader can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }

  // Create the reader.
  svtkPLYReader* reader = svtkPLYReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkPNGReader* readerImg = svtkPNGReader::New();
  if (0 == readerImg->CanReadFile(fnameImg))
  {
    std::cout << "The PNG reader can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }
  readerImg->SetFileName(fnameImg);
  readerImg->Update();
  delete[] fnameImg;

  // Create the texture.
  svtkTexture* texture = svtkTexture::New();
  texture->SetInputConnection(readerImg->GetOutputPort());

  // Create a mapper.
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  // Basic visualisation.
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(400, 400);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  actor->Delete();
  mapper->Delete();
  reader->Delete();
  readerImg->Delete();
  texture->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
