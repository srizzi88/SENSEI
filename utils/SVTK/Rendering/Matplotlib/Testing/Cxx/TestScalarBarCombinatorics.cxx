/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarBarCombinatorics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkBandedPolyDataContourFilter.h"
#include "svtkColorSeries.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiBlockPLOT3DReader.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridGeometryFilter.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkTextProperty.h"

#include <stdlib.h> // for atof

struct svtkScalarBarTestCondition
{
  const char* Title;
  int Orientation;
  int TextPosition;
  int DrawAnnotations;
  int DrawNanAnnotation;
  int IndexedLookup;
  int FixedAnnotationLeaderLineColor;
  double Position[2];
  double Position2[2];
  int ProcessEvents;
  int Enabled;
  int VTitleSeparation;
} conditions[] = {
  { "$T_1$", SVTK_ORIENT_HORIZONTAL, svtkScalarBarActor::PrecedeScalarBar, 1, 1, 1, 0,
    { 0.000, 0.015 }, { 0.400, 0.135 }, 1, 1, 0 },
  { "$T_2$", SVTK_ORIENT_HORIZONTAL, svtkScalarBarActor::PrecedeScalarBar, 1, 0, 1, 1,
    { 0.000, 0.230 }, { 0.400, 0.146 }, 1, 1, 0 },
  { "$T_3$", SVTK_ORIENT_HORIZONTAL, svtkScalarBarActor::SucceedScalarBar, 1, 1, 1, 1,
    { 0.000, 0.850 }, { 0.630, 0.154 }, 1, 1, 5 },
  { "$T_4$", SVTK_ORIENT_VERTICAL, svtkScalarBarActor::PrecedeScalarBar, 1, 1, 1, 0, { 0.799, 0.032 },
    { 0.061, 0.794 }, 1, 1, 5 },
  { "$T_5$", SVTK_ORIENT_VERTICAL, svtkScalarBarActor::PrecedeScalarBar, 1, 0, 1, 1, { 0.893, 0.036 },
    { 0.052, 0.752 }, 1, 1, 0 },
  { "$T_6$", SVTK_ORIENT_VERTICAL, svtkScalarBarActor::SucceedScalarBar, 1, 1, 1, 1, { 0.792, 0.081 },
    { 0.061, 0.617 }, 1, 1, 0 },
  { "$T_7$", SVTK_ORIENT_VERTICAL, svtkScalarBarActor::SucceedScalarBar, 1, 1, 0, 0, { 0.646, 0.061 },
    { 0.084, 0.714 }, 1, 1, 0 },
  { "$T_8$", SVTK_ORIENT_HORIZONTAL, svtkScalarBarActor::SucceedScalarBar, 0, 1, 0, 1,
    { 0.076, 0.535 }, { 0.313, 0.225 }, 1, 1, 0 },
};

static svtkSmartPointer<svtkScalarBarActor> CreateScalarBar(svtkScalarBarTestCondition& cond,
  svtkScalarsToColors* idxLut, svtkScalarsToColors* conLut, svtkRenderer* ren)
{
  svtkNew<svtkScalarBarActor> sba;
  sba->SetTitle(cond.Title);
  sba->SetLookupTable(cond.IndexedLookup ? idxLut : conLut);
  sba->SetOrientation(cond.Orientation);
  sba->SetTextPosition(cond.TextPosition);
  sba->SetDrawAnnotations(cond.DrawAnnotations);
  sba->SetDrawNanAnnotation(cond.DrawNanAnnotation);
  sba->SetFixedAnnotationLeaderLineColor(cond.FixedAnnotationLeaderLineColor);
  sba->SetPosition(cond.Position[0], cond.Position[1]);
  sba->SetPosition2(cond.Position2[0], cond.Position2[1]);
  sba->SetVerticalTitleSeparation(cond.VTitleSeparation);
  ren->AddActor(sba);
  return sba;
}

int TestScalarBarCombinatorics(int argc, char* argv[])
{
  svtkTesting* t = svtkTesting::New();
  double threshold = 10.;
  for (int cc = 1; cc < argc; ++cc)
  {
    if ((cc < argc - 1) && (argv[cc][0] == '-') && (argv[cc][1] == 'E'))
    {
      threshold = atof(argv[++cc]);
      continue;
    }
    t->AddArgument(argv[cc]);
  }

  svtkNew<svtkRenderer> ren1;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkLookupTable> lutA;
  svtkNew<svtkLookupTable> lutB;
  // Create a grid of scalar bars
  int numBars = static_cast<int>(sizeof(conditions) / sizeof(conditions[0]));
  std::vector<svtkSmartPointer<svtkScalarBarActor> > actors;
  actors.reserve(numBars);
  for (int c = 0; c < numBars; ++c)
  {
    actors.push_back(CreateScalarBar(conditions[c], lutA, lutB, ren1));
  }

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 300);

  svtkNew<svtkColorSeries> pal;
  pal->SetColorSchemeByName("Brewer Sequential Blue-Green (5)");
  pal->BuildLookupTable(lutB);
  lutB->IndexedLookupOff();
  lutB->Build();
  lutB->SetAnnotation(5.00, "Just Wow");
  lutB->SetAnnotation(4.00, "Super-Special");
  lutB->SetAnnotation(3.00, "Amazingly Special");
  lutB->SetAnnotation(1.00, "Special");
  lutB->SetAnnotation(0.00, "Special $\\cap$ This $= \\emptyset$");
  lutB->SetRange(0., 4.); // Force "Just Wow" to be omitted from rendering.
  lutB->Build();

  // Now make a second set of annotations with an even number of entries (10).
  // This tests another branch of the annotation label positioning code.
  pal->SetColorSchemeByName("Brewer Diverging Purple-Orange (10)");
  pal->BuildLookupTable(lutA);
  lutA->SetAnnotation(5.00, "A");
  lutA->SetAnnotation(4.00, "B");
  lutA->SetAnnotation(3.00, "C");
  lutA->SetAnnotation(2.00, "D");
  lutA->SetAnnotation(1.00, ""); // Test empty label omission
  lutA->SetAnnotation(0.00, "F");
  lutA->SetAnnotation(6.00, "G");
  lutA->SetAnnotation(7.00, "H");
  lutA->SetAnnotation(8.00, "I");
  lutA->SetAnnotation(9.00, ""); // Test empty label omission

  // render the image
  iren->Initialize();
  renWin->Render();
  t->SetRenderWindow(renWin);
  int res = t->RegressionTest(threshold);
  t->Delete();

  iren->Start();

  return res == svtkTesting::PASSED ? 0 : 1;
}
