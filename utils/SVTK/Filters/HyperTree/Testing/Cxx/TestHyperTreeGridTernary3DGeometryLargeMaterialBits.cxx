/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DGeometryMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Joachim Pouderoux, Kitware 2013
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkBitArray.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkHyperTreeGrid.h"
#include "svtkIdTypeArray.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"
#include <svtkObjectFactory.h>

// Define interaction style
class KeyPressInteractorStyle : public svtkInteractorStyleTrackballCamera
{
public:
  static KeyPressInteractorStyle* New();
  svtkTypeMacro(KeyPressInteractorStyle, svtkInteractorStyleTrackballCamera);

  void OnKeyPress() override
  {
    // Get the keypress
    svtkRenderWindowInteractor* rwi = this->Interactor;
    std::string key = rwi->GetKeySym();

    // Handle a "normal" key
    if (key == "a")
    {
      double* pos = this->Renderer->GetActiveCamera()->GetPosition();
      double* focal = this->Renderer->GetActiveCamera()->GetFocalPoint();
      double* clip = this->Renderer->GetActiveCamera()->GetClippingRange();
      double* up = this->Renderer->GetActiveCamera()->GetViewUp();
      cout << "----" << endl;
      cout << "Camera position " << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;
      cout << "Camera focalpoint " << focal[0] << ", " << focal[1] << ", " << focal[2] << endl;
      cout << "Camera viewup " << up[0] << ", " << up[1] << ", " << up[2] << endl;
      cout << "Camera range " << clip[0] << ", " << clip[1] << endl;
    }

    // Forward events
    svtkInteractorStyleTrackballCamera::OnKeyPress();
  }

  svtkRenderer* Renderer;
};
svtkStandardNewMacro(KeyPressInteractorStyle);

int TestHyperTreeGridTernary3DGeometryLargeMaterialBits(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(6);
  htGrid->SetDimensions(101, 101, 21); // GridCell 100, 100, 20
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->UseMaskOn();
  const std::string descriptor =
    ".RRR.RR..R.R .R|" // Level 0 refinement
    "R.......................... ........................... ........................... "
    ".............R............. ....RR.RR........R......... .....RRRR.....R.RR......... "
    "........................... ...........................|........................... "
    "........................... ........................... ...RR.RR.......RR.......... "
    "........................... RR......................... ........................... "
    "........................... ........................... ........................... "
    "........................... ........................... ........................... "
    "............RRR............|........................... ........................... "
    ".......RR.................. ........................... ........................... "
    "........................... ........................... ........................... "
    "........................... ........................... "
    "...........................|........................... ...........................";
  const std::string materialMask = // Level 0 materials are not needed, visible cells are described
                                   // with LevelZeroMaterialIndex
    "111111111111111111111111111 000000000100110111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 000110011100000100100010100|000001011011111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111001111111101111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111|000000000111100100111100100 000000000111001001111001001 "
    "000000111100100111111111111 000000111001001111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 "
    "110110110100111110111000000|111111111111111111111111111 111111111111111111111111111";
  svtkIdType zeroArray[] = { 0, 1, 2, 4, 5, 7, 8, 9, 30, 29 * 30 + 1, 30 * 30, 30 * 30 * 19,
    30 * 30 * 20 - 2, 30 * 30 * 20 - 1 };
  svtkNew<svtkIdTypeArray> zero;
  zero->SetArray(zeroArray, sizeof(zeroArray) / sizeof(svtkIdType), 1, 0);
  htGrid->SetLevelZeroMaterialIndex(zero);
  svtkBitArray* desc = htGrid->ConvertDescriptorStringToBitArray(descriptor);
  htGrid->SetDescriptorBits(desc);
  desc->Delete();
  svtkBitArray* mat = htGrid->ConvertMaskStringToBitArray(materialMask);
  htGrid->SetMaskBits(mat);
  mat->Delete();
  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
  cout << "Tree created in " << timer->GetElapsedTime() << "s" << endl;
  // DDM htGrid->GetHyperTreeGridOutput()->GetNumberOfCells();

  timer->StartTimer();
  // Geometry
  svtkNew<svtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  svtkPolyData* pd = geometry->GetPolyDataOutput();
  timer->StopTimer();
  cout << "Geometry computed in " << timer->GetElapsedTime() << "s" << endl;

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  // Renderer
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  // Camera
  renderer->GetActiveCamera()->SetFocalPoint(39.47, 14.97, 5.83);
  renderer->GetActiveCamera()->SetPosition(-34.83, -20.41, -27.78);
  renderer->GetActiveCamera()->SetViewUp(-0.257301, 0.959041, -0.118477);
  renderer->GetActiveCamera()->SetClippingRange(0.314716, 314.716);

  // Render window
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  svtkNew<KeyPressInteractorStyle> style;
  style->Renderer = renderer;
  iren->SetInteractorStyle(style);

  // Render and test
  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 30);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
