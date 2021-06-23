/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkJPEGReader.h>
#include <svtkMath.h>
#include <svtkNew.h>
#include <svtkOBJReader.h>
#include <svtkOBJWriter.h>
#include <svtkPNGReader.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTestUtilities.h>
#include <svtkTexture.h>
#include <svtkTexturedSphereSource.h>

#include <svtkNumberToString.h>

#include <string>

int TestOBJPolyDataWriter(int argc, char* argv[])
{
  svtkNew<svtkTexturedSphereSource> sphereSource;
  sphereSource->SetThetaResolution(16);
  sphereSource->SetPhiResolution(16);

  svtkNew<svtkJPEGReader> textReader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NE2_ps_bath_small.jpg");
  textReader->SetFileName(fname);
  delete[] fname;

  char* tname =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string filename = tmpDir + "/TestOBJPolyDataWriter_write.obj";

  svtkNew<svtkOBJWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetInputConnection(0, sphereSource->GetOutputPort());
  writer->SetInputConnection(1, textReader->GetOutputPort());
  writer->Write();

  svtkPolyData* polyInput = sphereSource->GetOutput();

  // read this file and compare with input
  svtkNew<svtkOBJReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  svtkPolyData* polyOutput = reader->GetOutput();

  if (polyInput->GetNumberOfPoints() != polyOutput->GetNumberOfPoints())
  {
    cerr << "PolyData do not have the same number of points.\n";
    return EXIT_FAILURE;
  }

  svtkDataArray* positionsInput = polyInput->GetPoints()->GetData();
  svtkDataArray* positionsOutput = polyOutput->GetPoints()->GetData();
  svtkDataArray* normalsInput = polyInput->GetPointData()->GetNormals();
  svtkDataArray* normalsOutput = polyOutput->GetPointData()->GetNormals();
  svtkDataArray* tcoordsInput = polyInput->GetPointData()->GetTCoords();
  svtkDataArray* tcoordsOutput = polyOutput->GetPointData()->GetTCoords();

  if (!positionsInput || !positionsOutput || !normalsInput || !normalsOutput || !tcoordsInput ||
    !tcoordsOutput)
  {
    cerr << "One of the arrays is null.\n";
    return EXIT_FAILURE;
  }

  // check values
  svtkNumberToString convert;
  int numberOfDifferentPoints = 0;
  int numberOfDifferentNormals = 0;
  int numberOfDifferentTCoords = 0;
  for (svtkIdType i = 0; i < polyInput->GetNumberOfPoints(); i++)
  {
    double pi[3], po[3];

    // check positions
    positionsInput->GetTuple(i, pi);
    positionsOutput->GetTuple(i, po);
    if (svtkMath::Distance2BetweenPoints(pi, po) > 0.0)
    {
      cerr << "Point is different.\n";
      cerr << "  Input:  " << convert(pi[0]) << " " << convert(pi[1]) << " " << convert(pi[2])
           << "\n";
      cerr << "  Output: " << convert(po[0]) << " " << convert(po[1]) << " " << convert(po[2])
           << "\n";
      numberOfDifferentPoints++;
    }

    // check normals
    normalsInput->GetTuple(i, pi);
    normalsOutput->GetTuple(i, po);
    if (svtkMath::AngleBetweenVectors(pi, po) > 0)
    {
      cerr << "Normal is different:\n";
      cerr << "  Input:  " << convert(pi[0]) << " " << convert(pi[1]) << " " << convert(pi[2])
           << "\n";
      cerr << "  Output: " << convert(po[0]) << " " << convert(po[1]) << " " << convert(po[2])
           << "\n";
      numberOfDifferentNormals++;
    }

    // check texture coords
    tcoordsInput->GetTuple(i, pi);
    tcoordsOutput->GetTuple(i, po);
    pi[2] = po[2] = 0.0;
    if (svtkMath::Distance2BetweenPoints(pi, po) > 0.0)
    {
      cerr << "Texture coord is different:\n";
      cerr << "Input:  " << convert(pi[0]) << " " << convert(pi[1]) << "\n";
      cerr << "Output: " << convert(po[0]) << " " << convert(po[1]) << "\n";
      numberOfDifferentTCoords++;
    }
  }
  if (numberOfDifferentPoints != 0 || numberOfDifferentNormals != 0 ||
    numberOfDifferentTCoords != 0)
  {
    return EXIT_FAILURE;
  }

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // read png file and set up texture
  svtkNew<svtkPNGReader> pngReader;
  std::string pngFile = filename.replace(filename.length() - 3, 3, "png");
  pngReader->SetFileName(pngFile.c_str());

  svtkNew<svtkTexture> texture;
  texture->SetInputConnection(pngReader->GetOutputPort());

  // add mapper and texture in an actor
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  // Standard rendering classes
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  renderer->AddActor(actor);
  renderer->ResetCamera();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
