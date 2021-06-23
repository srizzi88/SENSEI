/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkAbstractElectronicData.h"
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkImageData.h"
#include "svtkImageShiftScale.h"
#include "svtkMolecule.h"
#include "svtkMoleculeMapper.h"
#include "svtkNew.h"
#include "svtkOpenQubeMoleculeSource.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimpleBondPerceiver.h"
#include "svtkSmartPointer.h"
#include "svtkSmartVolumeMapper.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include <openqube/basisset.h>
#include <openqube/basissetloader.h>

int TestOpenQubeMOPACOrbital(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/2h2o.aux");

  svtkNew<svtkOpenQubeMoleculeSource> oq;
  oq->SetFileName(fname);
  oq->Update();

  delete[] fname;

  svtkSmartPointer<svtkMolecule> mol = svtkSmartPointer<svtkMolecule>::New();
  mol = oq->GetOutput();

  // If there aren't any bonds, attempt to perceive them
  if (mol->GetNumberOfBonds() == 0)
  {
    cout << "No bonds found. Running simple bond perception...\n";
    svtkNew<svtkSimpleBondPerceiver> bonder;
    bonder->SetInputData(mol);
    bonder->Update();
    mol = bonder->GetOutput();
    cout << "Bonds found: " << mol->GetNumberOfBonds() << "\n";
  }

  svtkNew<svtkMoleculeMapper> molMapper;
  molMapper->SetInputData(mol);
  molMapper->UseLiquoriceStickSettings();
  molMapper->SetBondRadius(0.1);
  molMapper->SetAtomicRadiusScaleFactor(0.1);

  svtkNew<svtkActor> molActor;
  molActor->SetMapper(molMapper);

  svtkAbstractElectronicData* edata = oq->GetOutput()->GetElectronicData();
  if (!edata)
  {
    cout << "null svtkAbstractElectronicData returned from "
            "svtkOpenQubeElectronicData.\n";
    return EXIT_FAILURE;
  }

  cout << "Num electrons: " << edata->GetNumberOfElectrons() << "\n";

  svtkSmartPointer<svtkImageData> data = svtkSmartPointer<svtkImageData>::New();
  data = edata->GetMO(4);
  if (!data)
  {
    cout << "null svtkImageData returned from svtkOpenQubeElectronicData.\n";
    return EXIT_FAILURE;
  }

  double range[2];
  data->GetScalarRange(range);
  cout << "ImageData range: " << range[0] << " " << range[1] << "\n";
  double maxAbsVal = (fabs(range[0]) > fabs(range[1])) ? fabs(range[0]) : fabs(range[1]);

  svtkNew<svtkImageShiftScale> t;
  t->SetInputData(data);
  t->SetShift(maxAbsVal);
  double magnitude = maxAbsVal + maxAbsVal;
  if (fabs(magnitude) < 1e-10)
    magnitude = 1.0;
  t->SetScale(255.0 / magnitude);
  t->SetOutputScalarTypeToDouble();

  cout << "magnitude: " << magnitude << "\n";

  t->Update();
  t->GetOutput()->GetScalarRange(range);
  cout << "Shifted min/max: " << range[0] << " " << range[1] << "\n";

  svtkNew<svtkPiecewiseFunction> compositeOpacity;
  compositeOpacity->AddPoint(0.00, 1.0);
  compositeOpacity->AddPoint(63.75, 0.8);
  compositeOpacity->AddPoint(127.50, 0.0);
  compositeOpacity->AddPoint(191.25, 0.8);
  compositeOpacity->AddPoint(255.00, 1.0);

  svtkNew<svtkColorTransferFunction> color;
  color->AddRGBSegment(0.00, 1.0, 0.0, 0.0, 127.0, 1.0, 0.0, 0.0);
  color->AddRGBSegment(128.0, 0.0, 0.0, 1.0, 255.0, 0.0, 0.0, 1.0);

  svtkNew<svtkSmartVolumeMapper> volumeMapper;
  volumeMapper->SetInputConnection(t->GetOutputPort());
  volumeMapper->SetBlendModeToComposite();

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetScalarOpacity(compositeOpacity);
  volumeProperty->SetColor(color);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(volume);
  ren->AddActor(molActor);

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(2.4);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
