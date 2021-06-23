/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCityGMLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of an RGBA texture on a svtkActor.
// .SECTION Description
// this program tests the CityGML Reader and setting of textures to
// individual datasets of the multiblock tree.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCityGMLReader.h"
#include "svtkCompositeDataIterator.h"
#include "svtkFieldData.h"
#include "svtkJPEGReader.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"
#include "svtksys/SystemTools.hxx"

int TestCityGMLReader(int argc, char* argv[])
{
  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CityGML/Part-4-Buildings-V4-one.gml");

  std::cout << fname << std::endl;
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  svtkNew<svtkCityGMLReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  svtkMultiBlockDataSet* mb = reader->GetOutput();

  svtkSmartPointer<svtkCompositeDataIterator> it;
  for (it.TakeReference(mb->NewIterator()); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    svtkPolyData* poly = svtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (poly)
    {

      svtkNew<svtkPolyDataMapper> mapper;
      mapper->SetInputDataObject(poly);

      svtkNew<svtkActor> actor;
      actor->SetMapper(mapper);
      renderer->AddActor(actor);
      svtkStringArray* textureField =
        svtkStringArray::SafeDownCast(poly->GetFieldData()->GetAbstractArray("texture_uri"));
      if (textureField)
      {
        std::string fnamePath = svtksys::SystemTools::GetFilenamePath(std::string(fname));

        const char* textureURI = textureField->GetValue(0);
        svtkNew<svtkJPEGReader> JpegReader;
        JpegReader->SetFileName((fnamePath + "/" + textureURI).c_str());
        JpegReader->Update();

        svtkNew<svtkTexture> texture;
        texture->SetInputConnection(JpegReader->GetOutputPort());
        texture->InterpolateOn();

        actor->SetTexture(texture);
      }
    }
  }

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(90);
  renderer->GetActiveCamera()->Roll(-90);
  renderer->GetActiveCamera()->Zoom(1.5);

  renWin->SetSize(400, 400);
  renWin->Render();
  interactor->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  delete[] fname;
  return !retVal;
}
