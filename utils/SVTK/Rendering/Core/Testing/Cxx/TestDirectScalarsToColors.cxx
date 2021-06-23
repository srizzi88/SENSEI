/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDirectScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor2D.h"
#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkImageMapper.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarsToColors.h"
#include "svtkShortArray.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedShortArray.h"

#include <cmath>

namespace
{
template <typename T>
void UCharToColor(unsigned char src, T* dest)
{
  *dest = src;
}

template <>
inline void UCharToColor(unsigned char src, double* dest)
{
  *dest = ((static_cast<double>(src) / 255.0));
}

template <>
inline void UCharToColor(unsigned char src, float* dest)
{
  *dest = (static_cast<float>(src) / 255.0);
}
};

template <typename T, typename BaseT>
void addViews(svtkRenderWindow* renWin, int typeIndex)
{
  svtkNew<svtkScalarsToColors> map;
  // Make the four sets of test scalars
  svtkSmartPointer<T> inputs[4];
  for (int ncomp = 1; ncomp <= 4; ncomp++)
  {
    int posX = ((ncomp - 1) & 1);
    int posY = ((ncomp - 1) >> 1);
    inputs[ncomp - 1] = svtkSmartPointer<T>::New();
    T* arr = inputs[ncomp - 1];

    arr->SetNumberOfComponents(ncomp);
    arr->SetNumberOfTuples(6400);

    // luminance conversion factors
    static const float a = 0.30;
    static const float b = 0.59;
    static const float c = 0.11;
    static const float d = 0.50;
    static const int f = 85;

    BaseT cval[4];
    svtkIdType i = 0;
    for (int j = 0; j < 16; j++)
    {
      for (int jj = 0; jj < 5; jj++)
      {
        for (int k = 0; k < 16; k++)
        {
          cval[0] = (((k >> 2) & 3) * f);
          cval[1] = ((k & 3) * f);
          cval[2] = (((j >> 2) & 3) * f);
          cval[3] = ((j & 3) * f);
          float l = cval[0] * a + cval[1] * b + cval[2] * c + d;
          unsigned char lc = static_cast<unsigned char>(l);
          cval[0] = ((ncomp > 2 ? cval[0] : lc));
          cval[1] = ((ncomp > 2 ? cval[1] : cval[3]));
          // store values between 0 and 1 for floating point colors.
          for (int index = 0; index < 4; ++index)
          {
            UCharToColor(cval[index], &cval[index]);
          }
          for (int kk = 0; kk < 5; kk++)
          {
            arr->SetTypedTuple(i++, cval);
          }
        }
      }
    }

    svtkNew<svtkImageData> image;
    image->SetDimensions(80, 80, 1);
    svtkUnsignedCharArray* colors = map->MapScalars(arr, SVTK_COLOR_MODE_DIRECT_SCALARS, -1);
    if (colors == nullptr)
    {
      continue;
    }
    image->GetPointData()->SetScalars(colors);
    colors->Delete();

    int pos[2];
    pos[0] = (((typeIndex & 3) << 1) + posX) * 80;
    pos[1] = ((((typeIndex >> 2) & 3) << 1) + posY) * 80;

    svtkNew<svtkImageMapper> mapper;
    mapper->SetColorWindow(255.0);
    mapper->SetColorLevel(127.5);
    mapper->SetInputData(image);

    svtkNew<svtkActor2D> actor;
    actor->SetMapper(mapper);

    svtkNew<svtkRenderer> ren;
    ren->AddViewProp(actor);
    ren->SetViewport(pos[0] / 640.0, pos[1] / 640.0, (pos[0] + 80) / 640.0, (pos[1] + 80) / 640.0);

    renWin->AddRenderer(ren);
  }
}

// Modified from TestBareScalarsToColors
int TestDirectScalarsToColors(int argc, char* argv[])
{
  // Cases to check:
  // 1, 2, 3, 4 components

  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->SetSize(640, 640);

  int i = -1;
  addViews<svtkUnsignedCharArray, unsigned char>(renWin, ++i);
  // This line generates an expected ERROR message.
  // addViews<svtkCharArray, char>(renWin, ++i);
  addViews<svtkUnsignedShortArray, unsigned short>(renWin, ++i);
  addViews<svtkShortArray, short>(renWin, ++i);
  addViews<svtkUnsignedIntArray, unsigned int>(renWin, ++i);
  addViews<svtkIntArray, int>(renWin, ++i);
  addViews<svtkUnsignedLongArray, unsigned long>(renWin, ++i);
  addViews<svtkLongArray, long>(renWin, ++i);
  addViews<svtkFloatArray, float>(renWin, ++i);
  addViews<svtkDoubleArray, double>(renWin, ++i);
  // Mac-Lion-64-gcc-4.2.1 (kamino) does not clear the render window
  // unless we create renderers for the whole window.
  for (++i; i < 16; ++i)
  {
    int pos[2];
    pos[0] = (i & 3) * 160;
    pos[1] = ((i >> 2) & 3) * 160;
    svtkNew<svtkRenderer> ren;
    ren->SetViewport(
      pos[0] / 640.0, pos[1] / 640.0, (pos[0] + 160) / 640.0, (pos[1] + 160) / 640.0);
    renWin->AddRenderer(ren);
  }

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
