/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractThresholdsMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests point, cell, and row selection and extraction from a multiblock data set
// made up of two svtkPolyDatas and svtkTable.

#include "svtkDoubleArray.h"
#include "svtkExtractSelection.h"
#include "svtkIdFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkMultiBlockDataGroupFilter.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkSelectionSource.h"
#include "svtkSphereSource.h"
#include "svtkTable.h"

int TestExtractThresholdsMultiBlock(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkSphereSource> sphere;

  // To test that the point precision matches in the extracted data
  // (default point precision is float).
  sphere->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  // Block 1: has PointId point data array
  svtkNew<svtkIdFilter> spherePointIDSource;
  spherePointIDSource->SetPointIdsArrayName("PointId");
  spherePointIDSource->PointIdsOn();
  spherePointIDSource->SetInputConnection(sphere->GetOutputPort());

  // Block 2: has CellId cell data array
  svtkNew<svtkIdFilter> sphereCellIDSource;
  sphereCellIDSource->SetCellIdsArrayName("CellId");
  sphereCellIDSource->CellIdsOn();
  sphereCellIDSource->SetInputConnection(sphere->GetOutputPort());

  // Block 3: table source with row data
  svtkNew<svtkTable> table;
  svtkNew<svtkDoubleArray> column1;
  column1->SetName("One");
  column1->SetNumberOfComponents(1);
  column1->SetNumberOfTuples(10);
  column1->FillValue(1);
  svtkNew<svtkDoubleArray> column2;
  column2->SetName("Three");
  column2->SetNumberOfComponents(1);
  column2->SetNumberOfTuples(10);
  column2->FillValue(3);
  table->AddColumn(column1);
  table->AddColumn(column2);

  // Create multiblock dataset
  svtkNew<svtkMultiBlockDataGroupFilter> group;
  group->AddInputConnection(spherePointIDSource->GetOutputPort());
  group->AddInputConnection(sphereCellIDSource->GetOutputPort());
  group->AddInputData(table);

  // Test point value threshold selection
  svtkNew<svtkSelectionNode> selectionNodePoints;
  selectionNodePoints->SetContentType(svtkSelectionNode::THRESHOLDS);
  selectionNodePoints->SetFieldType(svtkSelectionNode::POINT);
  svtkNew<svtkIdTypeArray> thresholdPoints;
  thresholdPoints->SetName("PointId");
  thresholdPoints->SetNumberOfComponents(2);
  thresholdPoints->SetNumberOfTuples(1);
  thresholdPoints->SetTypedComponent(0, 0, 10);
  thresholdPoints->SetTypedComponent(0, 1, 20);
  selectionNodePoints->SetSelectionList(thresholdPoints);

  svtkNew<svtkSelection> selectionPoints;
  selectionPoints->AddNode(selectionNodePoints);

  svtkNew<svtkExtractSelection> extractPoints;
  extractPoints->SetInputConnection(0, group->GetOutputPort());
  extractPoints->SetInputData(1, selectionPoints);
  extractPoints->PreserveTopologyOff();
  extractPoints->Update();

  auto extracted = svtkMultiBlockDataSet::SafeDownCast(extractPoints->GetOutput());
  if (!extracted)
  {
    std::cerr << "Output was not a svtkMultiBlockDataSet." << std::endl;
    return EXIT_FAILURE;
  }
  if (!extracted->GetBlock(0) || extracted->GetBlock(1) || extracted->GetBlock(2))
  {
    std::cerr << "Blocks were not as expected" << std::endl;
    return EXIT_FAILURE;
  }
  if (svtkDataSet::SafeDownCast(extracted->GetBlock(0))->GetNumberOfPoints() != 11)
  {
    std::cerr << "Unexpected number of points in extracted selection" << std::endl;
    return EXIT_FAILURE;
  }

  // Test cell value threshold selection
  svtkNew<svtkSelectionNode> selectionNodeCells;
  selectionNodeCells->SetContentType(svtkSelectionNode::THRESHOLDS);
  selectionNodeCells->SetFieldType(svtkSelectionNode::CELL);
  svtkNew<svtkIdTypeArray> thresholdCells;
  thresholdCells->SetName("CellId");
  thresholdCells->SetNumberOfComponents(2);
  thresholdCells->SetNumberOfTuples(1);
  thresholdCells->SetTypedComponent(0, 0, 10);
  thresholdCells->SetTypedComponent(0, 1, 20);
  selectionNodeCells->SetSelectionList(thresholdCells);

  svtkNew<svtkSelection> selectionCells;
  selectionCells->AddNode(selectionNodeCells);

  svtkNew<svtkExtractSelection> extractCells;
  extractCells->SetInputConnection(0, group->GetOutputPort());
  extractCells->SetInputData(1, selectionCells);
  extractCells->PreserveTopologyOff();
  extractCells->Update();

  extracted = svtkMultiBlockDataSet::SafeDownCast(extractCells->GetOutput());
  if (!extracted)
  {
    std::cerr << "Output was not a svtkMultiBlockDataSet." << std::endl;
    return EXIT_FAILURE;
  }
  if (extracted->GetBlock(0) || !extracted->GetBlock(1) || extracted->GetBlock(2))
  {
    std::cerr << "Blocks were not as expected" << std::endl;
    return EXIT_FAILURE;
  }
  if (svtkDataSet::SafeDownCast(extracted->GetBlock(1))->GetNumberOfCells() != 11)
  {
    std::cerr << "Unexpected number of cells in extracted selection" << std::endl;
    return EXIT_FAILURE;
  }
  if (!svtkPointSet::SafeDownCast(extracted->GetBlock(1)))
  {
    std::cerr << "Block 1 was not a svtkPointSet, but a " << extracted->GetBlock(1)->GetClassName()
              << " instead." << std::endl;
    return EXIT_FAILURE;
  }
  if (svtkPointSet::SafeDownCast(extracted->GetBlock(1))->GetPoints()->GetData()->GetDataType() !=
    SVTK_DOUBLE)
  {
    std::cerr << "Output for block 1 should have points with double precision" << std::endl;
    return EXIT_FAILURE;
  }

  // Test table value threshold selection
  svtkNew<svtkSelectionNode> selectionNodeRows;
  selectionNodeRows->SetContentType(svtkSelectionNode::THRESHOLDS);
  selectionNodeRows->SetFieldType(svtkSelectionNode::ROW);
  svtkNew<svtkDoubleArray> thresholdRows;
  thresholdRows->SetName("One");
  thresholdRows->SetNumberOfComponents(2);
  thresholdRows->SetNumberOfTuples(1);
  thresholdRows->SetTypedComponent(0, 0, 0.0);
  thresholdRows->SetTypedComponent(0, 1, 10.0);
  selectionNodeRows->SetSelectionList(thresholdRows);

  svtkNew<svtkSelection> selectionRows;
  selectionRows->AddNode(selectionNodeRows);

  svtkNew<svtkExtractSelection> extractRows;
  extractRows->SetInputConnection(0, group->GetOutputPort());
  extractRows->SetInputData(1, selectionRows);
  extractRows->PreserveTopologyOff();
  extractRows->Update();

  extracted = svtkMultiBlockDataSet::SafeDownCast(extractRows->GetOutput());
  if (!extracted)
  {
    std::cerr << "Output was not a svtkMultiBlockDataSet." << std::endl;
    return EXIT_FAILURE;
  }
  if (extracted->GetBlock(0) || extracted->GetBlock(1) || !extracted->GetBlock(2))
  {
    std::cerr << "Blocks were not as expected" << std::endl;
    return EXIT_FAILURE;
  }
  if (svtkTable::SafeDownCast(extracted->GetBlock(2))->GetNumberOfRows() != 10)
  {
    std::cerr << "Unexpected number of rows in extracted selection" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
