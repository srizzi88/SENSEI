/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDataTypesMIP.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test volume renders the same dataset using 8 different data types
// (unsigned char, signed char, unsigned short, signed short, unsigned int
// int, float, and double). It uses compositing and no shading. The extents of
// the dataset are (0, 114, 0, 100, 0, 74).
//
// Tests cell-data with a large data type (int).

#include "svtkAlgorithmOutput.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageShiftScale.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointDataToCellData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTransform.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLImageDataReader.h"

namespace
{
typedef struct
{
  svtkSmartPointer<svtkGPUVolumeRayCastMapper> volumeMapper[4][4];
  svtkSmartPointer<svtkVolumeProperty> volumeProperty[4][4];
  svtkSmartPointer<svtkVolume> volume[4][4];
  svtkSmartPointer<svtkTransform> userMatrix[4][4];
  svtkSmartPointer<svtkImageShiftScale> shiftScale[4][4];
  svtkSmartPointer<svtkAlgorithmOutput> algoOut[4][4];
  svtkSmartPointer<svtkColorTransferFunction> color[4][4];
  svtkSmartPointer<svtkPiecewiseFunction> opacity[4][4];
} SVTKData;

void RegisterVolumeToRender(SVTKData& data, svtkRenderer* ren1, const int i, const int j)
{
  data.volumeMapper[i][j] = svtkSmartPointer<svtkGPUVolumeRayCastMapper>::New();
  data.volumeMapper[i][j]->SetBlendModeToMaximumIntensity();
  data.volumeMapper[i][j]->SetInputConnection(data.algoOut[i][j]);

  data.volumeProperty[i][j] = svtkSmartPointer<svtkVolumeProperty>::New();
  data.volumeProperty[i][j]->SetColor(data.color[i][j]);
  data.volumeProperty[i][j]->SetScalarOpacity(data.opacity[i][j]);
  data.volumeProperty[i][j]->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  data.volume[i][j] = svtkSmartPointer<svtkVolume>::New();
  data.volume[i][j]->SetMapper(data.volumeMapper[i][j]);
  data.volume[i][j]->SetProperty(data.volumeProperty[i][j]);

  data.userMatrix[i][j] = svtkSmartPointer<svtkTransform>::New();
  data.userMatrix[i][j]->PostMultiply();
  data.userMatrix[i][j]->Identity();
  data.userMatrix[i][j]->Translate(i * 120, j * 120, 0);

  data.volume[i][j]->SetUserTransform(data.userMatrix[i][j]);
  ren1->AddViewProp(data.volume[i][j]);
}
}

int TestGPURayCastDataTypesMIP(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");

  SVTKData data;

  svtkNew<svtkXMLImageDataReader> reader;
  reader->SetFileName(cfname);
  delete[] cfname;

  // unsigned char
  data.shiftScale[0][0] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[0][0]->SetInputConnection(reader->GetOutputPort());
  data.algoOut[0][0] = data.shiftScale[0][0]->GetOutputPort();

  data.color[0][0] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[0][0]->AddRGBPoint(0, 0, 0, 1);
  data.color[0][0]->AddRGBPoint(255, 0, 1, 0);

  data.opacity[0][0] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[0][0]->AddPoint(0, 0);
  data.opacity[0][0]->AddPoint(255, 1);

  // unsigned char (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_02;
  pointsToCells_02->SetInputConnection(data.shiftScale[0][0]->GetOutputPort());
  data.algoOut[0][2] = pointsToCells_02->GetOutputPort();

  data.color[0][2] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[0][2]->AddRGBPoint(0, 0, 0, 1);
  data.color[0][2]->AddRGBPoint(255, 0, 1, 0);

  data.opacity[0][2] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[0][2]->AddPoint(0, 0);
  data.opacity[0][2]->AddPoint(255, 1);

  // signed char
  data.shiftScale[0][1] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[0][1]->SetInputConnection(data.shiftScale[0][0]->GetOutputPort());
  data.shiftScale[0][1]->SetShift(-128);
  data.shiftScale[0][1]->SetOutputScalarType(15);
  data.algoOut[0][1] = data.shiftScale[0][1]->GetOutputPort();

  data.color[0][1] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[0][1]->AddRGBPoint(-128, 0, 0, 1);
  data.color[0][1]->AddRGBPoint(127, 0, 1, 0);

  data.opacity[0][1] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[0][1]->AddPoint(-128, 0);
  data.opacity[0][1]->AddPoint(127, 1);

  // signed char (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_03;
  pointsToCells_03->SetInputConnection(data.shiftScale[0][1]->GetOutputPort());
  data.algoOut[0][3] = pointsToCells_03->GetOutputPort();

  data.color[0][3] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[0][3]->AddRGBPoint(-128, 0, 0, 1);
  data.color[0][3]->AddRGBPoint(127, 0, 1, 0);

  data.opacity[0][3] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[0][3]->AddPoint(-128, 0);
  data.opacity[0][3]->AddPoint(127, 1);

  // unsigned short
  data.shiftScale[1][0] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[1][0]->SetInputConnection(reader->GetOutputPort());
  data.shiftScale[1][0]->SetScale(256);
  data.shiftScale[1][0]->SetOutputScalarTypeToUnsignedShort();
  data.algoOut[1][0] = data.shiftScale[1][0]->GetOutputPort();

  data.color[1][0] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[1][0]->AddRGBPoint(0, 0, 0, 1);
  data.color[1][0]->AddRGBPoint(65535, 0, 1, 0);

  data.opacity[1][0] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[1][0]->AddPoint(0, 0);
  data.opacity[1][0]->AddPoint(65535, 1);

  // unsigned short (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_12;
  pointsToCells_12->SetInputConnection(data.shiftScale[1][0]->GetOutputPort());
  data.algoOut[1][2] = pointsToCells_12->GetOutputPort();

  data.color[1][2] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[1][2]->AddRGBPoint(0, 0, 0, 1);
  data.color[1][2]->AddRGBPoint(65535, 0, 1, 0);

  data.opacity[1][2] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[1][2]->AddPoint(0, 0);
  data.opacity[1][2]->AddPoint(65535, 1);

  //  short
  data.shiftScale[1][1] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[1][1]->SetInputConnection(data.shiftScale[1][0]->GetOutputPort());
  data.shiftScale[1][1]->SetShift(-32768);
  data.shiftScale[1][1]->SetOutputScalarTypeToShort();
  data.algoOut[1][1] = data.shiftScale[1][1]->GetOutputPort();

  data.color[1][1] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[1][1]->AddRGBPoint(-32768, 0, 0, 1);
  data.color[1][1]->AddRGBPoint(32767, 0, 1, 0);

  data.opacity[1][1] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[1][1]->AddPoint(-32768, 0);
  data.opacity[1][1]->AddPoint(32767, 1);

  //  short (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_13;
  pointsToCells_13->SetInputConnection(data.shiftScale[1][1]->GetOutputPort());
  data.algoOut[1][3] = pointsToCells_13->GetOutputPort();

  data.color[1][3] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[1][3]->AddRGBPoint(-32768, 0, 0, 1);
  data.color[1][3]->AddRGBPoint(32767, 0, 1, 0);

  data.opacity[1][3] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[1][3]->AddPoint(-32768, 0);
  data.opacity[1][3]->AddPoint(32767, 1);

  // unsigned int
  data.shiftScale[2][0] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[2][0]->SetInputConnection(reader->GetOutputPort());
  data.shiftScale[2][0]->SetScale(16777216);
  data.shiftScale[2][0]->SetOutputScalarTypeToUnsignedInt();

  data.color[2][0] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[2][0]->AddRGBPoint(0, 0, 0, 1);
  data.color[2][0]->AddRGBPoint(SVTK_UNSIGNED_INT_MAX, 0, 1, 0);
  data.algoOut[2][0] = data.shiftScale[2][0]->GetOutputPort();

  data.opacity[2][0] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[2][0]->AddPoint(0, 0);
  data.opacity[2][0]->AddPoint(SVTK_UNSIGNED_INT_MAX, 1);

  // unsigned int (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_22;
  pointsToCells_22->SetInputConnection(data.shiftScale[2][0]->GetOutputPort());
  data.algoOut[2][2] = pointsToCells_22->GetOutputPort();

  data.color[2][2] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[2][2]->AddRGBPoint(0, 0, 0, 1);
  data.color[2][2]->AddRGBPoint(SVTK_UNSIGNED_INT_MAX, 0, 1, 0);

  data.opacity[2][2] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[2][2]->AddPoint(0, 0);
  data.opacity[2][2]->AddPoint(SVTK_UNSIGNED_INT_MAX, 1);

  // int
  data.shiftScale[2][1] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[2][1]->SetInputConnection(data.shiftScale[2][0]->GetOutputPort());
  data.shiftScale[2][1]->SetShift(SVTK_INT_MIN);
  data.shiftScale[2][1]->SetOutputScalarTypeToInt();
  data.algoOut[2][1] = data.shiftScale[2][1]->GetOutputPort();

  data.color[2][1] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[2][1]->AddRGBPoint(SVTK_INT_MIN, 0, 0, 1);
  data.color[2][1]->AddRGBPoint(SVTK_INT_MAX, 0, 1, 0);

  data.opacity[2][1] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[2][1]->AddPoint(SVTK_INT_MIN, 0);
  data.opacity[2][1]->AddPoint(SVTK_INT_MAX, 1);

  // int (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells;
  pointsToCells->SetInputConnection(data.shiftScale[2][1]->GetOutputPort());
  data.algoOut[2][3] = pointsToCells->GetOutputPort();

  data.color[2][3] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[2][3]->AddRGBPoint(SVTK_INT_MIN, 0, 0, 1);
  data.color[2][3]->AddRGBPoint(SVTK_INT_MAX, 0, 1, 0);

  data.opacity[2][3] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[2][3]->AddPoint(SVTK_INT_MIN, 0);
  data.opacity[2][3]->AddPoint(SVTK_INT_MAX, 1);

  // float [-1 1]
  svtkNew<svtkImageShiftScale> shiftScale_3_0_pre;
  shiftScale_3_0_pre->SetInputConnection(reader->GetOutputPort());
  shiftScale_3_0_pre->SetScale(0.0078125);
  shiftScale_3_0_pre->SetOutputScalarTypeToFloat();

  data.shiftScale[3][0] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[3][0]->SetInputConnection(shiftScale_3_0_pre->GetOutputPort());
  data.shiftScale[3][0]->SetShift(-1.0);
  data.shiftScale[3][0]->SetOutputScalarTypeToFloat();
  data.algoOut[3][0] = data.shiftScale[3][0]->GetOutputPort();

  data.color[3][0] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[3][0]->AddRGBPoint(-1.0, 0, 0, 1);
  data.color[3][0]->AddRGBPoint(1.0, 0, 1, 0);

  data.opacity[3][0] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[3][0]->AddPoint(-1.0, 0);
  data.opacity[3][0]->AddPoint(1.0, 1);

  // float (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_32;
  pointsToCells_32->SetInputConnection(data.shiftScale[3][0]->GetOutputPort());
  data.algoOut[3][2] = pointsToCells_32->GetOutputPort();

  data.color[3][2] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[3][2]->AddRGBPoint(-1.0, 0, 0, 1);
  data.color[3][2]->AddRGBPoint(1.0, 0, 1, 0);

  data.opacity[3][2] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[3][2]->AddPoint(-1.0, 0);
  data.opacity[3][2]->AddPoint(1.0, 1);

  // double [-1000 3000]
  svtkNew<svtkImageShiftScale> shiftScale_3_1_pre;
  shiftScale_3_1_pre->SetInputConnection(reader->GetOutputPort());
  shiftScale_3_1_pre->SetScale(15.625);
  shiftScale_3_1_pre->SetOutputScalarTypeToDouble();

  data.shiftScale[3][1] = svtkSmartPointer<svtkImageShiftScale>::New();
  data.shiftScale[3][1]->SetInputConnection(shiftScale_3_1_pre->GetOutputPort());
  data.shiftScale[3][1]->SetShift(-1000);
  data.shiftScale[3][1]->SetOutputScalarTypeToDouble();
  data.algoOut[3][1] = data.shiftScale[3][1]->GetOutputPort();

  data.color[3][1] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[3][1]->AddRGBPoint(-1000, 0, 0, 1);
  data.color[3][1]->AddRGBPoint(3000, 0, 1, 0);

  data.opacity[3][1] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[3][1]->AddPoint(-1000, 0);
  data.opacity[3][1]->AddPoint(3000, 1);

  // double (cell data)
  svtkNew<svtkPointDataToCellData> pointsToCells_33;
  pointsToCells_33->SetInputConnection(data.shiftScale[3][1]->GetOutputPort());
  data.algoOut[3][3] = pointsToCells_33->GetOutputPort();

  data.color[3][3] = svtkSmartPointer<svtkColorTransferFunction>::New();
  data.color[3][3]->AddRGBPoint(-1000, 0, 0, 1);
  data.color[3][3]->AddRGBPoint(3000, 0, 1, 0);

  data.opacity[3][3] = svtkSmartPointer<svtkPiecewiseFunction>::New();
  data.opacity[3][3]->AddPoint(-1000, 0);
  data.opacity[3][3]->AddPoint(3000, 1);

  svtkNew<svtkRenderer> ren1;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->SetSize(600, 600);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  int i = 0;
  while (i < 4)
  {
    int j = 0;
    while (j < 4)
    {
      RegisterVolumeToRender(data, ren1, i, j);
      ++j;
    }
    ++i;
  }

  int valid = data.volumeMapper[0][1]->IsRenderSupported(renWin, data.volumeProperty[0][1]);
  int retVal;
  if (valid)
  {
    iren->Initialize();
    ren1->SetBackground(0.1, 0.4, 0.2);
    ren1->ResetCamera();
    ren1->GetActiveCamera()->Zoom(1.25);
    renWin->Render();

    retVal = svtkTesting::Test(argc, argv, renWin, 75);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  else
  {
    retVal = svtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
  }

  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
