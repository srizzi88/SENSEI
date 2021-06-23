/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenGLPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test different sizes with svtkTexture
// .SECTION Description
// This program tests 1D and 2D texture sizes.

#include <svtkCellArray.h>
#include <svtkFloatArray.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPNGWriter.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper2D.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTexture.h>
#include <svtkTexturedActor2D.h>

svtkImageData* createTexture2D(int width, int height, int comp)
{
  void* data = malloc(width * height * comp * sizeof(unsigned char));
  if (!data)
  {
    return nullptr;
  }
  free(data);
  svtkImageData* image = svtkImageData::New();
  image->SetExtent(0, width - 1, 0, height - 1, 0, 0);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, comp);

  unsigned char* ptr = reinterpret_cast<unsigned char*>(image->GetScalarPointer(0, 0, 0));
  double value = 0.;
  double valueIncr = 255. / (width * height > 1 ? width * height - 1 : 1);
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      for (int c = 0; c < comp; ++c)
      {
        *ptr++ = static_cast<unsigned char>(value);
      }
      value += valueIncr;
    }
  }
  return image;
}

int TestTextureSize(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create a renderer, render window, and interactor
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  svtkNew<svtkPoints> points;
  points->InsertPoint(0, 0., 0., 0.);
  points->InsertPoint(1, 200., 0., 0.);
  points->InsertPoint(2, 200., 200., 0.);
  points->InsertPoint(3, 0., 200., 0.);

  svtkNew<svtkCellArray> cells;
  cells->InsertNextCell(4);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);

  svtkNew<svtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->InsertNextTuple2(0.f, 0.f);
  tcoords->InsertNextTuple2(1.f, 0.f);
  tcoords->InsertNextTuple2(1.f, 1.f);
  tcoords->InsertNextTuple2(0.f, 1.f);

  svtkNew<svtkPolyData> textureCoords;
  textureCoords->SetPoints(points);
  textureCoords->SetPolys(cells);
  textureCoords->GetPointData()->SetTCoords(tcoords);

  svtkNew<svtkPolyDataMapper2D> polyDataMapper;
  polyDataMapper->SetInputData(textureCoords);

  int textureSizes[23][2] = { { 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 255 }, { 1, 256 },
    { 257, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 255, 1 }, { 256, 1 }, { 257, 1 },
    { 1, 1 }, { 2, 2 }, { 3, 3 }, { 3, 3 }, { 255, 255 }, { 256, 256 }, { 257, 257 },
    { 2047, 2047 }, { 4097, 4097 } };
  int componentSizes[3] = { 1, 3, 4 };
  for (int i = 0; i < 23; ++i)
  {
    for (int c = 0; c < 3; ++c)
    {
      int* size = textureSizes[i];
      svtkSmartPointer<svtkImageData> image =
        svtkSmartPointer<svtkImageData>::Take(createTexture2D(size[0], size[1], componentSizes[c]));
      if (image == nullptr)
      {
        return EXIT_SUCCESS;
      }
      svtkNew<svtkTexture> texture;
      texture->SetInputData(image);
      // You can play with the parameters
      // texture->SetRepeat(false);
      // texture->SetEdgeClamp(true);
      // texture->SetInterpolate(true);

      svtkNew<svtkTexturedActor2D> textureActor;
      textureActor->SetTexture(texture);
      textureActor->SetMapper(polyDataMapper);
      renderer->AddActor(textureActor);

      texture->SetRestrictPowerOf2ImageSmaller(false);
      renderWindow->Render();

      texture->SetRestrictPowerOf2ImageSmaller(true);
      renderWindow->Render();
    }
  }

  return EXIT_SUCCESS;
}
