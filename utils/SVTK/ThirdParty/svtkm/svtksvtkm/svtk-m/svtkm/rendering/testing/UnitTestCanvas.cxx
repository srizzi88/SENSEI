//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/Testing.h>
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  svtkm::rendering::Canvas canvas;
  canvas.SetBackgroundColor(svtkm::rendering::Color::white);
  canvas.Initialize();
  canvas.Activate();
  canvas.Clear();
  canvas.AddLine(-0.8, 0.8, 0.8, 0.8, 1.0f, svtkm::rendering::Color::black);
  canvas.AddLine(0.8, 0.8, 0.8, -0.8, 1.0f, svtkm::rendering::Color::black);
  canvas.AddLine(0.8, -0.8, -0.8, -0.8, 1.0f, svtkm::rendering::Color::black);
  canvas.AddLine(-0.8, -0.8, -0.8, 0.8, 1.0f, svtkm::rendering::Color::black);
  canvas.AddLine(-0.8, -0.8, 0.8, 0.8, 1.0f, svtkm::rendering::Color::black);
  canvas.AddLine(-0.8, 0.8, 0.8, -0.8, 1.0f, svtkm::rendering::Color::black);
  svtkm::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  canvas.AddColorBar(colorBarBounds, svtkm::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  canvas.SaveAs("canvas.pnm");
}

} //namespace

int UnitTestCanvas(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
