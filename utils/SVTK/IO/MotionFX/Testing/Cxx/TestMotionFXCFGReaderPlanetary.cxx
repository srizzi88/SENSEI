/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMotionFXCFGReaderPlanetary.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestMotionFXCFGReaderCommon.h"

#include <svtkCamera.h>
#include <svtkInformation.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkStreamingDemandDrivenPipeline.h>
#include <svtkTestUtilities.h>

#include <vector>

int TestMotionFXCFGReaderPlanetary(int argc, char* argv[])
{
  return impl::Test(argc, argv, "Data/MotionFX/planetary/Planetary_prescribedCOMmotion.cfg",
    [](svtkRenderWindow*, svtkRenderer* renderer, svtkMotionFXCFGReader*) {
      auto camera = renderer->GetActiveCamera();
      camera->SetFocalPoint(-412.84, 121.00, -304.88);
      camera->SetPosition(-412.17, 121.27, -305.32);
      camera->SetViewUp(0.54, 0.032, 0.83);
      renderer->ResetCamera();
    });
}
