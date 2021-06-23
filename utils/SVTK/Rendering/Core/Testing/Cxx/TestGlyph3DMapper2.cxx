/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test case of empty input for svtkGlyph3DMapper. Refer to MR!1529.
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"
#include <svtkActor.h>
#include <svtkCellArray.h>
#include <svtkCubeSource.h>
#include <svtkGlyph3DMapper.h>
#include <svtkMath.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

int TestGlyph3DMapper2(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // create empty input data
  svtkSmartPointer<svtkPolyData> polydata = svtkSmartPointer<svtkPolyData>::New();

  svtkSmartPointer<svtkCubeSource> cubeSource = svtkSmartPointer<svtkCubeSource>::New();

  svtkSmartPointer<svtkGlyph3DMapper> glyph3Dmapper = svtkSmartPointer<svtkGlyph3DMapper>::New();
  glyph3Dmapper->SetSourceConnection(cubeSource->GetOutputPort());
  glyph3Dmapper->SetInputData(polydata);
  glyph3Dmapper->Update();

  double boundsAnswer[6];
  svtkMath::UninitializeBounds(boundsAnswer);
  // since there is nothing inside the scene, the boundsResult should be an
  // uninitializeBounds
  const double* boundsResult = glyph3Dmapper->GetBounds();
  for (int i = 0; i < 6; ++i)
  {
    if (boundsResult[i] != boundsAnswer[i])
      return -1;
  }
  return 0;
}
