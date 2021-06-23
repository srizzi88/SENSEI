/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkAppendPolyData.h>
#include <svtkCellArray.h>
#include <svtkSmartPointer.h>
#include <svtkXMLPolyDataWriter.h>

int TestAppendPolyData(int, char*[])
{
  svtkSmartPointer<svtkPoints> pointsArray0 = svtkSmartPointer<svtkPoints>::New();
  pointsArray0->InsertNextPoint(0.0, 0.0, 0.0);
  pointsArray0->InsertNextPoint(1.0, 1.0, 1.0);

  svtkSmartPointer<svtkPoints> pointsArray1 = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> vertices = svtkSmartPointer<svtkCellArray>::New();
  svtkIdType pointIds[1];
  pointIds[0] = pointsArray1->InsertNextPoint(5.0, 5.0, 5.0);
  vertices->InsertNextCell(1, pointIds);
  pointIds[0] = pointsArray1->InsertNextPoint(6.0, 6.0, 6.0);
  vertices->InsertNextCell(1, pointIds);

  svtkSmartPointer<svtkPolyData> inputPolyData0 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points0 = svtkSmartPointer<svtkPoints>::New();
  points0->SetDataType(SVTK_FLOAT);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  svtkSmartPointer<svtkXMLPolyDataWriter> inputWriter0 = svtkSmartPointer<svtkXMLPolyDataWriter>::New();
  inputWriter0->SetFileName("inputpolydata0.vtp");
  inputWriter0->SetInputData(inputPolyData0);
  inputWriter0->Write();

  svtkSmartPointer<svtkPolyData> inputPolyData1 = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points1 = svtkSmartPointer<svtkPoints>::New();
  points1->SetDataType(SVTK_FLOAT);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);
  inputPolyData1->SetVerts(vertices);

  svtkSmartPointer<svtkXMLPolyDataWriter> inputWriter1 = svtkSmartPointer<svtkXMLPolyDataWriter>::New();
  inputWriter1->SetFileName("inputpolydata1.vtp");
  inputWriter1->SetInputData(inputPolyData1);
  inputWriter1->Write();

  svtkSmartPointer<svtkAppendPolyData> appendPolyData = svtkSmartPointer<svtkAppendPolyData>::New();
  appendPolyData->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);

  appendPolyData->AddInputData(inputPolyData0);
  appendPolyData->AddInputData(inputPolyData1);

  appendPolyData->Update();

  svtkSmartPointer<svtkPolyData> outputPolyData = appendPolyData->GetOutput();
  svtkSmartPointer<svtkXMLPolyDataWriter> outputWriter = svtkSmartPointer<svtkXMLPolyDataWriter>::New();
  outputWriter->SetFileName("outputpolydata.vtp");
  outputWriter->SetInputData(outputPolyData);
  outputWriter->Write();

  svtkSmartPointer<svtkAppendPolyData> appendPolyDataWithNoCells =
    svtkSmartPointer<svtkAppendPolyData>::New();
  appendPolyDataWithNoCells->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);

  appendPolyDataWithNoCells->AddInputData(inputPolyData0);
  appendPolyDataWithNoCells->AddInputData(inputPolyData0);

  appendPolyDataWithNoCells->Update();

  svtkSmartPointer<svtkPolyData> outputPolyDataWithNoCells = appendPolyDataWithNoCells->GetOutput();
  svtkSmartPointer<svtkXMLPolyDataWriter> outputWriterWithNoCells =
    svtkSmartPointer<svtkXMLPolyDataWriter>::New();
  outputWriterWithNoCells->SetFileName("outputpolydataWithNoCells.vtp");
  outputWriterWithNoCells->SetInputData(outputPolyDataWithNoCells);
  outputWriterWithNoCells->Write();

  if (outputPolyData->GetNumberOfPoints() !=
    inputPolyData0->GetNumberOfPoints() + inputPolyData1->GetNumberOfPoints())
  {
    std::cerr << "ERROR: The output number of points should be "
              << inputPolyData0->GetNumberOfPoints() + inputPolyData1->GetNumberOfPoints()
              << " but is " << outputPolyData->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyData->GetNumberOfCells() !=
    inputPolyData0->GetNumberOfCells() + inputPolyData1->GetNumberOfCells())
  {
    std::cerr << "ERROR: The output number of cells should be "
              << inputPolyData0->GetNumberOfCells() + inputPolyData1->GetNumberOfCells()
              << " but is " << outputPolyData->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyData->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    std::cerr << "ERROR: The output points data should be " << SVTK_FLOAT << " but is "
              << outputPolyData->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyDataWithNoCells->GetNumberOfPoints() != inputPolyData0->GetNumberOfPoints() * 2)
  {
    std::cerr << "ERROR: The output number of points should be "
              << inputPolyData0->GetNumberOfPoints() * 2 << " but is "
              << outputPolyDataWithNoCells->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyDataWithNoCells->GetNumberOfCells() != 0)
  {
    std::cerr << "ERROR The output number of cells should be 0 but is "
              << " but is " << outputPolyDataWithNoCells->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyDataWithNoCells->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    std::cerr << "ERROR: The output points data type should be " << SVTK_FLOAT << " but is "
              << outputPolyDataWithNoCells->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points0->SetDataType(SVTK_DOUBLE);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  appendPolyData->Update();

  outputPolyData = appendPolyData->GetOutput();

  if (outputPolyData->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    std::cerr << "ERROR: The output points data type should be " << SVTK_DOUBLE << " but is "
              << outputPolyData->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points1->SetDataType(SVTK_DOUBLE);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    std::cerr << "ERROR: The output points data type should be " << SVTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  appendPolyData->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  points0->SetDataType(SVTK_FLOAT);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  points1->SetDataType(SVTK_FLOAT);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    std::cerr << "ERROR: The output points data type should be " << SVTK_FLOAT << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points0->SetDataType(SVTK_DOUBLE);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    std::cerr << "The output points data type should be " << SVTK_FLOAT << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points1->SetDataType(SVTK_DOUBLE);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    std::cerr << "The output points data type should be " << SVTK_FLOAT << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  appendPolyData->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  points0->SetDataType(SVTK_FLOAT);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  points1->SetDataType(SVTK_FLOAT);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    std::cerr << "The output points data type should be " << SVTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points0->SetDataType(SVTK_DOUBLE);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    std::cerr << "The output points data type should be " << SVTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points1->SetDataType(SVTK_DOUBLE);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    std::cerr << "The output points data type should be " << SVTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
