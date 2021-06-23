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
#include "svtkMolecule.h"
#include "svtkMoleculeMapper.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestBondColorModeSingleColor(int, char*[])
{
  svtkNew<svtkMolecule> mol;

  mol->Initialize();

  svtkAtom O1 = mol->AppendAtom(8, 3.0088731969, 1.1344098673, 0.9985902874);
  svtkAtom O2 = mol->AppendAtom(8, -0.2616286966, 2.7806709534, 0.7027800226);
  svtkAtom C1 = mol->AppendAtom(6, -2.0738607910, 1.2298524695, 0.3421802228);
  svtkAtom C2 = mol->AppendAtom(6, -1.4140240045, 0.1045928523, 0.0352265378);
  svtkAtom C3 = mol->AppendAtom(6, 0.0000000000, 0.0000000000, 0.0000000000);
  svtkAtom C4 = mol->AppendAtom(6, 1.2001889412, 0.0000000000, 0.0000000000);
  svtkAtom C5 = mol->AppendAtom(6, -1.4612030913, 2.5403617582, 0.6885503164);
  svtkAtom C6 = mol->AppendAtom(6, 2.6528126498, 0.1432895796, 0.0427014196);
  svtkAtom H1 = mol->AppendAtom(1, -3.1589178142, 1.2268537165, 0.3536340040);
  svtkAtom H2 = mol->AppendAtom(1, -1.9782163251, -0.7930325394, -0.1986937306);
  svtkAtom H3 = mol->AppendAtom(1, 3.0459155564, 0.4511167867, -0.9307386568);
  svtkAtom H4 = mol->AppendAtom(1, 3.1371551056, -0.7952192984, 0.3266426961);
  svtkAtom H5 = mol->AppendAtom(1, 2.3344947615, 1.8381683043, 0.9310726537);
  svtkAtom H6 = mol->AppendAtom(1, -2.1991803919, 3.3206134015, 0.9413825084);

  mol->AppendBond(C1, C5, 1);
  mol->AppendBond(C1, C2, 2);
  mol->AppendBond(C2, C3, 1);
  mol->AppendBond(C3, C4, 3);
  mol->AppendBond(C4, C6, 1);
  mol->AppendBond(C5, O2, 2);
  mol->AppendBond(C6, O1, 1);
  mol->AppendBond(C5, H6, 1);
  mol->AppendBond(C1, H1, 1);
  mol->AppendBond(C2, H2, 1);
  mol->AppendBond(C6, H3, 1);
  mol->AppendBond(C6, H4, 1);
  mol->AppendBond(O1, H5, 1);

  svtkNew<svtkMoleculeMapper> molmapper;
  molmapper->SetInputData(mol);

  molmapper->UseBallAndStickSettings();
  molmapper->RenderAtomsOff();
  molmapper->SetBondColorModeToSingleColor();

  svtkNew<svtkActor> actor;
  actor->SetMapper(molmapper);

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(actor);

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(2.2);

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
