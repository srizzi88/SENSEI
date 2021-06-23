/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoryLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCategoryLegend.h"

#include "svtkColorSeries.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkVariantArray.h"

#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkContextView.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestCategoryLegend(int argc, char* argv[])
{
  svtkNew<svtkVariantArray> values;
  values->InsertNextValue(svtkVariant("a"));
  values->InsertNextValue(svtkVariant("b"));
  values->InsertNextValue(svtkVariant("c"));

  svtkNew<svtkLookupTable> lut;
  for (int i = 0; i < values->GetNumberOfTuples(); ++i)
  {
    lut->SetAnnotation(values->GetValue(i), values->GetValue(i).ToString());
  }

  svtkNew<svtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(svtkColorSeries::BREWER_QUALITATIVE_SET3);
  colorSeries->BuildLookupTable(lut);

  svtkNew<svtkCategoryLegend> legend;
  legend->SetScalarsToColors(lut);
  legend->SetValues(values);
  legend->SetTitle("legend");

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(legend);
  trans->Translate(180, 70);

  svtkNew<svtkContextView> contextView;
  contextView->GetScene()->AddItem(trans);
  contextView->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  contextView->GetRenderWindow()->SetSize(300, 200);
  contextView->GetRenderWindow()->SetMultiSamples(0);
  contextView->GetRenderWindow()->Render();

  int retVal = svtkRegressionTestImage(contextView->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    contextView->GetRenderWindow()->Render();
    contextView->GetInteractor()->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
