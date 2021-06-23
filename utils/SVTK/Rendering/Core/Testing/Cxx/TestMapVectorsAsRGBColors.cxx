/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMapVectorsAsRGBColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor2D.h"
#include "svtkImageData.h"
#include "svtkImageMapper.h"
#include "svtkLookupTable.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkUnsignedCharArray.h"

#include <cmath>

int TestMapVectorsAsRGBColors(int argc, char* argv[])
{
  // Cases to check:
  // 1, 2, 3, 4 components to 1, 2, 3, 4, components
  // with scaling and without scaling
  // with alpha and without alpha
  // so 64 tests in total
  // on an 8x8 grid

  // Make the four sets of test scalars
  svtkSmartPointer<svtkUnsignedCharArray> inputs[4];
  for (int ncomp = 1; ncomp <= 4; ncomp++)
  {
    inputs[ncomp - 1] = svtkSmartPointer<svtkUnsignedCharArray>::New();
    svtkUnsignedCharArray* arr = inputs[ncomp - 1];

    arr->SetNumberOfComponents(ncomp);
    arr->SetNumberOfTuples(6400);

    // luminance conversion factors
    static const float a = 0.30;
    static const float b = 0.59;
    static const float c = 0.11;
    static const float d = 0.50;
    static const int f = 85;

    unsigned char cval[4];
    svtkIdType i = 0;
    for (int j = 0; j < 16; j++)
    {
      for (int jj = 0; jj < 5; jj++)
      {
        for (int k = 0; k < 16; k++)
        {
          cval[0] = ((k >> 2) & 3) * f;
          cval[1] = (k & 3) * f;
          cval[2] = ((j >> 2) & 3) * f;
          cval[3] = (j & 3) * f;
          float l = cval[0] * a + cval[1] * b + cval[2] * c + d;
          unsigned char lc = static_cast<unsigned char>(l);
          cval[0] = (ncomp > 2 ? cval[0] : lc);
          cval[1] = (ncomp > 2 ? cval[1] : cval[3]);
          for (int kk = 0; kk < 5; kk++)
          {
            arr->SetTypedTuple(i++, cval);
          }
        }
      }
    }
  }

  svtkSmartPointer<svtkLookupTable> table = svtkSmartPointer<svtkLookupTable>::New();
  table->SetVectorModeToRGBColors();

  svtkSmartPointer<svtkLookupTable> table2 = svtkSmartPointer<svtkLookupTable>::New();

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renWin->SetSize(640, 640);

  // Make the 64 sets of output scalars
  svtkSmartPointer<svtkUnsignedCharArray> outputs[64];
  for (int i = 0; i < 64; i++)
  {
    int j = (i & 7);
    int k = ((i >> 3) & 7);
    double alpha = 0.5 * (2 - (j & 1));
    double range[2];
    range[0] = 63.75 * (k & 1);
    range[1] = 255.0 - 63.75 * (k & 1);
    int inputc = ((j >> 1) & 3) + 1;
    int outputc = ((k >> 1) & 3) + 1;

    table->SetRange(range);
    table->SetAlpha(alpha);

    outputs[i] = svtkSmartPointer<svtkUnsignedCharArray>::New();
    outputs[i]->SetNumberOfComponents(outputc);
    outputs[i]->SetNumberOfTuples(0);

    // test mapping with a count of zero
    svtkUnsignedCharArray* tmparray =
      table2->MapScalars(outputs[i], SVTK_COLOR_MODE_DEFAULT, outputc);
    tmparray->Delete();

    table->MapVectorsThroughTable(inputs[inputc - 1]->GetPointer(0),
      outputs[i]->WritePointer(0, 6400), SVTK_UNSIGNED_CHAR, 0, inputc, outputc);

    // now the real thing
    outputs[i]->SetNumberOfTuples(6400);

    table->MapVectorsThroughTable(inputs[inputc - 1]->GetPointer(0),
      outputs[i]->WritePointer(0, 6400), SVTK_UNSIGNED_CHAR, 6400, inputc, outputc);

    svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();
    image->SetDimensions(80, 80, 1);
    svtkUnsignedCharArray* colors = table2->MapScalars(outputs[i], SVTK_COLOR_MODE_DEFAULT, outputc);
    image->GetPointData()->SetScalars(colors);
    colors->Delete();

    int pos[2];
    pos[0] = j * 80;
    pos[1] = k * 80;

    svtkSmartPointer<svtkImageMapper> mapper = svtkSmartPointer<svtkImageMapper>::New();
    mapper->SetColorWindow(255.0);
    mapper->SetColorLevel(127.5);
    mapper->SetInputData(image);

    svtkSmartPointer<svtkActor2D> actor = svtkSmartPointer<svtkActor2D>::New();
    actor->SetMapper(mapper);

    svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
    ren->AddViewProp(actor);
    ren->SetViewport(pos[0] / 640.0, pos[1] / 640.0, (pos[0] + 80) / 640.0, (pos[1] + 80) / 640.0);

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
