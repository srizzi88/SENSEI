/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInteractorStyleImagePropertyMultiplePropSliceFirst.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor2D.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTextMapper.h"

int TestInteractorStyleImageProperty(int argc, char* argv[])
{
  svtkSmartPointer<svtkPNGReader> reader = svtkSmartPointer<svtkPNGReader>::New();

  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GreenCircle.png");
  reader->SetFileName(fileName);
  delete[] fileName;

  svtkSmartPointer<svtkImageSliceMapper> mapper = svtkSmartPointer<svtkImageSliceMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  svtkSmartPointer<svtkImageSlice> imageSlice = svtkSmartPointer<svtkImageSlice>::New();
  imageSlice->SetMapper(mapper);

  svtkSmartPointer<svtkImageProperty> property = svtkSmartPointer<svtkImageProperty>::New();
  property->SetColorWindow(4000);
  property->SetColorLevel(2000);

  imageSlice->SetProperty(property);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->ResetCamera();

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  svtkSmartPointer<svtkTextMapper> text = svtkSmartPointer<svtkTextMapper>::New();
  text->SetInput("Text");

  svtkSmartPointer<svtkActor2D> textActor = svtkSmartPointer<svtkActor2D>::New();
  textActor->SetMapper(text);
  textActor->PickableOff();

  renderer->AddViewProp(imageSlice);
  renderer->AddViewProp(textActor);

  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();
  style->SetCurrentRenderer(renderer);

  renderWindowInteractor->SetInteractorStyle(style);
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->Initialize();

  for (int sliceOrder = 0; sliceOrder < 4; sliceOrder++)
  {
    renderer->RemoveAllViewProps();

    switch (sliceOrder)
    {
      case 0:
        // Adding the slice to the renderer before the other prop.
        renderer->AddViewProp(imageSlice);
        renderer->AddViewProp(textActor);
        break;

      case 1:
        // Only add the slice if there should not be a prop.
        renderer->AddViewProp(imageSlice);
        break;

      case 2:
        // Adding the slice to the renderer after the other prop.
        renderer->AddViewProp(textActor);
        renderer->AddViewProp(imageSlice);
        break;

      case 3:
        // No slice, so no image property should be found.
        renderer->AddViewProp(textActor);
        break;
    }

    renderWindowInteractor->Render();

    // The StartWindowLevel event is not activated until the function
    // OnLeftButtonDown is called. Call it to force the event to trigger
    // the chain of methods to set the ImageProperty.
    style->OnLeftButtonDown();
    bool foundProperty = (style->GetCurrentImageProperty() == property);
    style->OnLeftButtonUp();

    if ((!foundProperty) ^ (sliceOrder == 3))
    {
      cerr << "TestInteractorStyleImagePropertyInternal failed with sliceOrder parameter "
           << sliceOrder << "." << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
