/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProcrustesAlignmentFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMultiBlockDataSet.h>
#include <svtkPolyData.h>
#include <svtkProcrustesAlignmentFilter.h>
#include <svtkSmartPointer.h>

int TestProcrustesAlignmentFilter(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkPoints> pointsArray[3];

  for (unsigned int i = 0; i < 3; ++i)
  {
    pointsArray[i] = svtkSmartPointer<svtkPoints>::New();
  }

  pointsArray[0]->InsertNextPoint(-1.58614838, -0.66562307, -0.20268087);
  pointsArray[0]->InsertNextPoint(-0.09052952, -1.53144991, 0.80403084);
  pointsArray[0]->InsertNextPoint(-1.17059791, 1.07974386, 0.68106824);
  pointsArray[0]->InsertNextPoint(0.32502091, 0.21391694, 1.68777990);
  pointsArray[0]->InsertNextPoint(-0.32502091, -0.21391694, -1.68777990);
  pointsArray[0]->InsertNextPoint(1.17059791, -1.07974386, -0.68106824);
  pointsArray[0]->InsertNextPoint(0.09052952, 1.53144991, -0.80403084);
  pointsArray[0]->InsertNextPoint(1.58614838, 0.66562307, 0.20268087);

  pointsArray[1]->InsertNextPoint(-1.58614838, -0.66562307, -0.20268087);
  pointsArray[1]->InsertNextPoint(-0.09052952, -1.53144991, 0.80403084);
  pointsArray[1]->InsertNextPoint(-1.17059791, 1.07974386, 0.68106824);
  pointsArray[1]->InsertNextPoint(0.32502091, 0.21391694, 1.68777990);
  pointsArray[1]->InsertNextPoint(-0.32502091, -0.21391694, -1.68777990);
  pointsArray[1]->InsertNextPoint(1.17059791, -1.07974386, -0.68106824);
  pointsArray[1]->InsertNextPoint(0.09052952, 1.53144991, -0.80403084);
  pointsArray[1]->InsertNextPoint(1.58614838, 0.66562307, 0.20268087);

  pointsArray[2]->InsertNextPoint(-1.58614838, -0.66562307, -0.20268087);
  pointsArray[2]->InsertNextPoint(-0.09052952, -1.53144991, 0.80403084);
  pointsArray[2]->InsertNextPoint(-1.17059791, 1.07974386, 0.68106824);
  pointsArray[2]->InsertNextPoint(0.32502091, 0.21391694, 1.68777990);
  pointsArray[2]->InsertNextPoint(-0.32502091, -0.21391694, -1.68777990);
  pointsArray[2]->InsertNextPoint(1.17059791, -1.07974386, -0.68106824);
  pointsArray[2]->InsertNextPoint(0.09052952, 1.53144991, -0.80403084);
  pointsArray[2]->InsertNextPoint(1.58614838, 0.66562307, 0.20268087);

  svtkSmartPointer<svtkMultiBlockDataSet> inputMultiBlockDataSet =
    svtkSmartPointer<svtkMultiBlockDataSet>::New();

  svtkSmartPointer<svtkProcrustesAlignmentFilter> procrustesAlignmentFilter =
    svtkSmartPointer<svtkProcrustesAlignmentFilter>::New();
  procrustesAlignmentFilter->SetInputData(inputMultiBlockDataSet);
  procrustesAlignmentFilter->StartFromCentroidOff();

  procrustesAlignmentFilter->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
    inputPoints->SetDataType(SVTK_FLOAT);
    inputPoints->DeepCopy(pointsArray[i]);

    svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
  }

  procrustesAlignmentFilter->Update();

  svtkSmartPointer<svtkPoints> meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if (meanPoints->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkMultiBlockDataSet> outputMultiBlockDataSet =
    procrustesAlignmentFilter->GetOutput();

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    svtkSmartPointer<svtkPolyData> outputPolyData = svtkPolyData::SafeDownCast(dataObject);
    svtkSmartPointer<svtkPoints> outputPoints = outputPolyData->GetPoints();

    if (outputPoints->GetDataType() != SVTK_FLOAT)
    {
      return EXIT_FAILURE;
    }
  }

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
    inputPoints->SetDataType(SVTK_DOUBLE);
    inputPoints->DeepCopy(pointsArray[i]);

    svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
  }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if (meanPoints->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    svtkSmartPointer<svtkPolyData> outputPolyData = svtkPolyData::SafeDownCast(dataObject);
    svtkSmartPointer<svtkPoints> outputPoints = outputPolyData->GetPoints();

    if (outputPoints->GetDataType() != SVTK_DOUBLE)
    {
      return EXIT_FAILURE;
    }
  }

  procrustesAlignmentFilter->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
    inputPoints->SetDataType(SVTK_FLOAT);
    inputPoints->DeepCopy(pointsArray[i]);

    svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
  }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if (meanPoints->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    svtkSmartPointer<svtkPolyData> outputPolyData = svtkPolyData::SafeDownCast(dataObject);
    svtkSmartPointer<svtkPoints> outputPoints = outputPolyData->GetPoints();

    if (outputPoints->GetDataType() != SVTK_FLOAT)
    {
      return EXIT_FAILURE;
    }
  }

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
    inputPoints->SetDataType(SVTK_DOUBLE);
    inputPoints->DeepCopy(pointsArray[i]);

    svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
  }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if (meanPoints->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    svtkSmartPointer<svtkPolyData> outputPolyData = svtkPolyData::SafeDownCast(dataObject);
    svtkSmartPointer<svtkPoints> outputPoints = outputPolyData->GetPoints();

    if (outputPoints->GetDataType() != SVTK_FLOAT)
    {
      return EXIT_FAILURE;
    }
  }

  procrustesAlignmentFilter->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
    inputPoints->SetDataType(SVTK_FLOAT);
    inputPoints->DeepCopy(pointsArray[i]);

    svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
  }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if (meanPoints->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    svtkSmartPointer<svtkPolyData> outputPolyData = svtkPolyData::SafeDownCast(dataObject);
    svtkSmartPointer<svtkPoints> outputPoints = outputPolyData->GetPoints();

    if (outputPoints->GetDataType() != SVTK_DOUBLE)
    {
      return EXIT_FAILURE;
    }
  }

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
    inputPoints->SetDataType(SVTK_DOUBLE);
    inputPoints->DeepCopy(pointsArray[i]);

    svtkSmartPointer<svtkPolyData> inputPolyData = svtkSmartPointer<svtkPolyData>::New();
    inputPolyData->SetPoints(inputPoints);

    inputMultiBlockDataSet->SetBlock(i, inputPolyData);
  }

  procrustesAlignmentFilter->Update();

  meanPoints = procrustesAlignmentFilter->GetMeanPoints();

  if (meanPoints->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  outputMultiBlockDataSet = procrustesAlignmentFilter->GetOutput();

  for (unsigned int i = 0; i < 3; ++i)
  {
    svtkSmartPointer<svtkDataObject> dataObject = outputMultiBlockDataSet->GetBlock(i);
    svtkSmartPointer<svtkPolyData> outputPolyData = svtkPolyData::SafeDownCast(dataObject);
    svtkSmartPointer<svtkPoints> outputPoints = outputPolyData->GetPoints();

    if (outputPoints->GetDataType() != SVTK_DOUBLE)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
