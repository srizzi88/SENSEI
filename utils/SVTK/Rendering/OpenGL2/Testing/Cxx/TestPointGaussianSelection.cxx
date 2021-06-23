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

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPointGaussianMapper.h"
#include "svtkPointSource.h"
#include "svtkProp3DCollection.h"
#include "svtkRandomAttributeGenerator.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedAreaPicker.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"

int TestPointGaussianSelection(int argc, char* argv[])
{
  int desiredPoints = 1.0e3;

  svtkNew<svtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints, 0.33) * 20.0);
  points->Update();

  svtkNew<svtkRandomAttributeGenerator> randomAttr;
  randomAttr->SetInputConnection(points->GetOutputPort());

  svtkNew<svtkPointGaussianMapper> mapper;

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

#ifdef TestPoints
  randomAttr->SetDataTypeToUnsignedChar();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->SetMinimumComponentValue(0);
  randomAttr->SetMaximumComponentValue(255);
  randomAttr->Update();
  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetScaleFactor(0.0);
  mapper->EmissiveOff();
#else
  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->Update();

  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointVectors");
  mapper->SetScaleArrayComponent(3);

  // Note that LookupTable is 4x faster than
  // ColorTransferFunction. So if you have a choice
  // Usa a lut instead.
  //
  svtkNew<svtkLookupTable> lut;
  lut->SetHueRange(0.1, 0.2);
  lut->SetSaturationRange(1.0, 0.5);
  lut->SetValueRange(0.8, 1.0);
  mapper->SetLookupTable(lut);
#endif

  renderWindow->Render();
  renderer->GetActiveCamera()->Zoom(3.5);
  renderWindow->Render();

  svtkNew<svtkHardwareSelector> selector;
  selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_POINTS);
  selector->SetRenderer(renderer);
  selector->SetArea(10, 10, 50, 50);
  svtkSelection* result = selector->Select();

  bool goodPick = false;

  if (result->GetNumberOfNodes() == 1)
  {
    svtkSelectionNode* node = result->GetNode(0);
    svtkIdTypeArray* selIds = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());

    if (selIds)
    {
      svtkIdType numIds = selIds->GetNumberOfTuples();
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        svtkIdType curId = selIds->GetValue(i);
        cerr << curId << "\n";
      }
    }

    if (node->GetProperties()->Has(svtkSelectionNode::PROP_ID()) &&
      node->GetProperties()->Get(svtkSelectionNode::PROP()) == actor.Get() &&
      node->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX()) == 1 && selIds &&
      selIds->GetNumberOfTuples() == 14 && selIds->GetValue(4) == 227)
    {
      goodPick = true;
    }
  }
  result->Delete();

  if (!goodPick)
  {
    cerr << "Incorrect splats picked!\n";
    return EXIT_FAILURE;
  }

  // Interact if desired
  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
