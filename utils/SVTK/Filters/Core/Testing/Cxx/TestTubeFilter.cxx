/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkCellArray.h>
#include <svtkJPEGReader.h>
#include <svtkMathUtilities.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkPolyLine.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkTexture.h>
#include <svtkTubeFilter.h>

namespace
{
void InitializePolyData(svtkPolyData* polyData, int dataType)
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();
  const int npts = 30;
  verts->InsertNextCell(npts);
  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  lines->InsertNextCell(npts);

  if (dataType == SVTK_DOUBLE)
  {
    points->SetDataType(SVTK_DOUBLE);
    for (unsigned int i = 0; i < npts; ++i)
    {
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      svtkIdType pointId = points->InsertNextPoint(point);
      verts->InsertCellPoint(pointId);
      lines->InsertCellPoint(pointId);
    }
  }
  else
  {
    points->SetDataType(SVTK_FLOAT);
    for (unsigned int i = 0; i < npts; ++i)
    {
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      svtkIdType pointId = points->InsertNextPoint(point);
      verts->InsertCellPoint(pointId);
      lines->InsertCellPoint(pointId);
    }
  }

  // Create a few duplicate point coordinates
  double point[3];
  // Same coordinates for point 0->4
  points->GetPoint(0, point);
  for (int i = 1; i < 5; i++)
  {
    points->SetPoint(i, point);
  }
  // Same coordinates for point 15->18
  points->GetPoint(15, point);
  for (int i = 16; i < 19; i++)
  {
    points->SetPoint(i, point);
  }

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
  lines->Squeeze();
  polyData->SetLines(lines);
}

int TubeFilter(int dataType, int outputPointsPrecision)
{
  svtkNew<svtkPolyData> inputPolyData;
  InitializePolyData(inputPolyData, dataType);

  svtkNew<svtkPolyData> originalInputPolyData;
  originalInputPolyData->DeepCopy(inputPolyData);

  svtkNew<svtkTubeFilter> tubeFilter;
  tubeFilter->SetOutputPointsPrecision(outputPointsPrecision);
  tubeFilter->SetInputData(inputPolyData);

  tubeFilter->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = tubeFilter->GetOutput();
  svtkSmartPointer<svtkPoints> points = outputPolyData->GetPoints();

  // Verify that the filter did not change the original input polydata
  svtkCellArray* originalLines = originalInputPolyData->GetLines();
  svtkCellArray* lines = inputPolyData->GetLines();
  if (originalLines->GetNumberOfCells() != lines->GetNumberOfCells())
  {
    std::cerr << "svtkTubeFilter corrupted input polydata number of lines: "
              << originalLines->GetNumberOfCells() << " != " << lines->GetNumberOfCells()
              << std::endl;
    return EXIT_FAILURE;
  }
  for (svtkIdType lineIndex = 0; lineIndex < originalLines->GetNumberOfCells(); lineIndex++)
  {
    svtkIdType originalNumberOfLinePoints = 0;
    const svtkIdType* originalLinePoints = nullptr;
    originalLines->GetCellAtId(lineIndex, originalNumberOfLinePoints, originalLinePoints);
    svtkIdType numberOfLinePoints = 0;
    const svtkIdType* linePoints = nullptr;
    lines->GetCellAtId(lineIndex, numberOfLinePoints, linePoints);
    if (originalNumberOfLinePoints != numberOfLinePoints)
    {
      std::cerr << "svtkTubeFilter corrupted input polydata number of lines: "
                << originalNumberOfLinePoints << " != " << numberOfLinePoints << std::endl;
      return EXIT_FAILURE;
    }
    for (svtkIdType pointIndex = 0; pointIndex < numberOfLinePoints; ++pointIndex)
    {
      if (originalLinePoints[pointIndex] != linePoints[pointIndex])
      {
        std::cerr << "svtkTubeFilter corrupted input polydata point indices:" << std::endl;
        for (svtkIdType pointIndexLog = 0; pointIndexLog < numberOfLinePoints; ++pointIndexLog)
        {
          std::cerr << "  " << originalLinePoints[pointIndexLog] << " -> "
                    << linePoints[pointIndexLog] << " "
                    << (originalLinePoints[pointIndexLog] == linePoints[pointIndexLog] ? "OK"
                                                                                       : "ERROR")
                    << std::endl;
        }
        return EXIT_FAILURE;
      }
    }
  }

  return points->GetDataType();
}

void TubeFilterGenerateTCoords(int generateTCoordsOption, svtkActor* tubeActor)
{
  // Define a polyline
  svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
  double pt0[3] = { 0.0, 1.0 + 2 * generateTCoordsOption, 0.0 };
  double pt1[3] = { 1.0, 0.0 + 2 * generateTCoordsOption, 0.0 };
  double pt2[3] = { 5.0, 0.0 + 2 * generateTCoordsOption, 0.0 };

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(pt0);
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);

  svtkSmartPointer<svtkPolyLine> polyLine = svtkSmartPointer<svtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(3);
  for (unsigned int i = 0; i < 3; i++)
  {
    polyLine->GetPointIds()->SetId(i, i);
  }

  svtkSmartPointer<svtkCellArray> cells = svtkSmartPointer<svtkCellArray>::New();
  cells->InsertNextCell(polyLine);

  inputPolyData->SetPoints(points);
  inputPolyData->SetLines(cells);

  // Define a tubeFilter
  svtkSmartPointer<svtkTubeFilter> tubeFilter = svtkSmartPointer<svtkTubeFilter>::New();
  tubeFilter->SetInputData(inputPolyData);
  tubeFilter->SetNumberOfSides(50);
  tubeFilter->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);
  tubeFilter->SetGenerateTCoords(generateTCoordsOption);

  if (generateTCoordsOption == SVTK_TCOORDS_FROM_LENGTH)
  {
    // Calculate the length of the input polydata to normalize texture coordinates
    double inputLength = 0.0;
    for (svtkIdType i = 0; i < inputPolyData->GetNumberOfPoints() - 1; i++)
    {
      double currentPt[3];
      inputPolyData->GetPoint(i, currentPt);

      double nextPt[3];
      inputPolyData->GetPoint(i + 1, nextPt);

      inputLength += sqrt(svtkMath::Distance2BetweenPoints(currentPt, nextPt));
    }
    tubeFilter->SetTextureLength(inputLength);
  }
  else if (generateTCoordsOption == SVTK_TCOORDS_FROM_SCALARS)
  {
    // Add a scalar array to the input polydata
    svtkSmartPointer<svtkIntArray> scalars = svtkSmartPointer<svtkIntArray>::New();
    scalars->SetName("ActiveScalars");
    svtkIdType nbPts = inputPolyData->GetNumberOfPoints();
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(nbPts);

    for (svtkIdType i = 0; i < nbPts; i++)
    {
      scalars->SetTuple1(i, i);
    }

    inputPolyData->GetPointData()->AddArray(scalars);
    inputPolyData->GetPointData()->SetActiveScalars("ActiveScalars");

    // Calculate tube filter texture length to normalize texture coordinates
    double range[3];
    scalars->GetRange(range);
    tubeFilter->SetTextureLength(range[1] - range[0]);
  }
  tubeFilter->Update();

  svtkSmartPointer<svtkPolyDataMapper> tubeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  tubeMapper->SetInputData(tubeFilter->GetOutput());

  tubeActor->SetMapper(tubeMapper);
}
}

int TestTubeFilter(int argc, char* argv[])
{
  int dataType = TubeFilter(SVTK_FLOAT, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(SVTK_DOUBLE, svtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(SVTK_FLOAT, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(SVTK_DOUBLE, svtkAlgorithm::SINGLE_PRECISION);

  if (dataType != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(SVTK_FLOAT, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TubeFilter(SVTK_DOUBLE, svtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  // Test GenerateTCoords
  char* textureFileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");
  svtkSmartPointer<svtkJPEGReader> JPEGReader = svtkSmartPointer<svtkJPEGReader>::New();
  JPEGReader->SetFileName(textureFileName);
  delete[] textureFileName;

  svtkSmartPointer<svtkTexture> texture = svtkSmartPointer<svtkTexture>::New();
  texture->SetInputConnection(JPEGReader->GetOutputPort());
  texture->InterpolateOn();
  texture->RepeatOff();
  texture->EdgeClampOn();

  svtkSmartPointer<svtkActor> tubeActor0 = svtkSmartPointer<svtkActor>::New();
  TubeFilterGenerateTCoords(SVTK_TCOORDS_FROM_NORMALIZED_LENGTH, tubeActor0);
  tubeActor0->SetTexture(texture);

  svtkSmartPointer<svtkActor> tubeActor1 = svtkSmartPointer<svtkActor>::New();
  TubeFilterGenerateTCoords(SVTK_TCOORDS_FROM_LENGTH, tubeActor1);
  tubeActor1->SetTexture(texture);

  svtkSmartPointer<svtkActor> tubeActor2 = svtkSmartPointer<svtkActor>::New();
  TubeFilterGenerateTCoords(SVTK_TCOORDS_FROM_SCALARS, tubeActor2);
  tubeActor2->SetTexture(texture);

  // Setup render window, renderer, and interactor
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  renderer->AddActor(tubeActor0);
  renderer->AddActor(tubeActor1);
  renderer->AddActor(tubeActor2);
  renderer->SetBackground(0.5, 0.5, 0.5);

  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
