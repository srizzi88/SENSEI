//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <GL/glew.h>
#include <svtkm/interop/testing/TestingTransferFancyHandles.h>
#include <svtkm/rendering/CanvasEGL.h>

int UnitTestFancyTransferEGL(int argc, char* argv[])
{
  svtkm::cont::Initialize(argc, argv);

  //get egl canvas to construct a context for us
  svtkm::rendering::CanvasEGL canvas(1024, 1024);
  canvas.Initialize();
  canvas.Activate();

  //get glew to bind all the opengl functions
  glewInit();

  return svtkm::interop::testing::TestingTransferFancyHandles::Run();
}
