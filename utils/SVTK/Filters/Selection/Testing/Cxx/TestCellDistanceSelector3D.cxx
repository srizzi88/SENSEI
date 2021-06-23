/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellDistanceSelector3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2012

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellDistanceSelector.h"
#include "svtkExtractSelection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"
#include "svtkUnstructuredGridWriter.h"

#include <sstream>

// Reference values
const svtkIdType cardCellDistanceSelection3D[] = {
  125,
  16,
  20,
  73,
};

// ------------------------------------------------------------------------------------------------
static int CheckExtractedUGrid(
  svtkExtractSelection* extract, const char* tag, int testIdx, bool writeGrid)
{
  // Output must be a multiblock dataset
  svtkMultiBlockDataSet* outputMB = svtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (!outputMB)
  {
    svtkGenericWarningMacro("Cannot downcast extracted selection to multiblock dataset.");

    return 1;
  }

  // First block must be an unstructured grid
  svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(outputMB->GetBlock(0));
  if (!ugrid)
  {
    svtkGenericWarningMacro("Cannot downcast extracted selection to unstructured grid.");

    return 1;
  }

  // Initialize test status
  int testStatus = 0;
  cerr << endl;

  // Verify selection cardinality
  svtkIdType nCells = ugrid->GetNumberOfCells();
  cout << tag << " contains " << nCells << " cells." << endl;

  if (nCells != cardCellDistanceSelection3D[testIdx])
  {
    svtkGenericWarningMacro(
      "Incorrect cardinality: " << nCells << " != " << cardCellDistanceSelection3D[testIdx]);
    testStatus = 1;
  }

  // Verify selection cells
  cerr << "Original cell Ids: ";
  ugrid->GetCellData()->SetActiveScalars("svtkOriginalCellIds");
  svtkDataArray* oCellIds = ugrid->GetCellData()->GetScalars();
  for (svtkIdType i = 0; i < oCellIds->GetNumberOfTuples(); ++i)
  {
    cerr << oCellIds->GetTuple1(i) << " ";
  }
  cerr << endl;

  // If requested, write mesh
  if (writeGrid)
  {
    std::ostringstream fileNameSS;
    fileNameSS << "./CellDistanceExtraction3D-" << testIdx << ".svtk";
    svtkSmartPointer<svtkUnstructuredGridWriter> writer =
      svtkSmartPointer<svtkUnstructuredGridWriter>::New();
    writer->SetFileName(fileNameSS.str().c_str());
    writer->SetInputData(ugrid);
    writer->Write();
    cerr << "Wrote file " << fileNameSS.str() << endl;
  }

  return testStatus;
}

//----------------------------------------------------------------------------
int TestCellDistanceSelector3D(int argc, char* argv[])
{
  // Initialize test value
  int testIntValue = 0;

  // Read 3D unstructured input mesh
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AngularSector.svtk");
  svtkSmartPointer<svtkUnstructuredGridReader> reader =
    svtkSmartPointer<svtkUnstructuredGridReader>::New();
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  // Create multi-block mesh for linear selector
  svtkSmartPointer<svtkMultiBlockDataSet> mesh = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  mesh->SetNumberOfBlocks(1);
  mesh->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Mesh");
  mesh->SetBlock(0, reader->GetOutput());

  // *****************************************************************************
  // 0. Selection within distance of 2 from cell 7010
  // *****************************************************************************

  // Create a selection, sel0, of cell with index 7010
  svtkSmartPointer<svtkIdTypeArray> selArr0 = svtkSmartPointer<svtkIdTypeArray>::New();
  selArr0->InsertNextValue(7010);
  svtkSmartPointer<svtkSelectionNode> selNode0 = svtkSmartPointer<svtkSelectionNode>::New();
  selNode0->SetContentType(svtkSelectionNode::INDICES);
  selNode0->SetFieldType(svtkSelectionNode::CELL);
  selNode0->GetProperties()->Set(svtkSelectionNode::COMPOSITE_INDEX(), 1);
  selNode0->SetSelectionList(selArr0);
  svtkSmartPointer<svtkSelection> sel0 = svtkSmartPointer<svtkSelection>::New();
  sel0->AddNode(selNode0);

  // Create selection up to topological distance of 2
  svtkSmartPointer<svtkCellDistanceSelector> ls0 = svtkSmartPointer<svtkCellDistanceSelector>::New();
  ls0->SetInputMesh(mesh);
  ls0->SetInputSelection(sel0);
  ls0->SetDistance(2);

  // Extract selection from mesh
  svtkSmartPointer<svtkExtractSelection> es0 = svtkSmartPointer<svtkExtractSelection>::New();
  es0->SetInputData(0, mesh);
  es0->SetInputConnection(1, ls0->GetOutputPort());
  es0->Update();
  testIntValue += CheckExtractedUGrid(es0, "Selection d({7010})<3", 0, true);

  // *****************************************************************************
  // 1. Selection at distance of 1 from ridge 7643-7499-7355-7211, excluding it
  // *****************************************************************************

  // Create a selection, sel1, of cells with indices 7643-7499-7355-7211
  svtkSmartPointer<svtkIdTypeArray> selArr1 = svtkSmartPointer<svtkIdTypeArray>::New();
  selArr1->InsertNextValue(7643);
  selArr1->InsertNextValue(7499);
  selArr1->InsertNextValue(7355);
  selArr1->InsertNextValue(7211);
  svtkSmartPointer<svtkSelectionNode> selNode1 = svtkSmartPointer<svtkSelectionNode>::New();
  selNode1->SetContentType(svtkSelectionNode::INDICES);
  selNode1->SetFieldType(svtkSelectionNode::CELL);
  selNode1->GetProperties()->Set(svtkSelectionNode::COMPOSITE_INDEX(), 1);
  selNode1->SetSelectionList(selArr1);
  svtkSmartPointer<svtkSelection> sel1 = svtkSmartPointer<svtkSelection>::New();
  sel1->AddNode(selNode1);

  // Create selection at distance of 1
  svtkSmartPointer<svtkCellDistanceSelector> ls1 = svtkSmartPointer<svtkCellDistanceSelector>::New();
  ls1->SetInputMesh(mesh);
  ls1->SetInputSelection(sel1);
  ls1->SetDistance(1);
  ls1->IncludeSeedOff();

  // Extract selection from mesh
  svtkSmartPointer<svtkExtractSelection> es1 = svtkSmartPointer<svtkExtractSelection>::New();
  es1->SetInputData(0, mesh);
  es1->SetInputConnection(1, ls1->GetOutputPort());
  es1->Update();
  testIntValue += CheckExtractedUGrid(es1, "Selection d({7643-7499-7355-7211})=1", 1, true);

  // *****************************************************************************
  // 2. Selection at distance of 2 from corner 7632, retaining seed
  // *****************************************************************************

  // Create a selection, sel2, of cell with index 7632
  svtkSmartPointer<svtkIdTypeArray> selArr2 = svtkSmartPointer<svtkIdTypeArray>::New();
  selArr2->InsertNextValue(7632);
  svtkSmartPointer<svtkSelectionNode> selNode2 = svtkSmartPointer<svtkSelectionNode>::New();
  selNode2->SetContentType(svtkSelectionNode::INDICES);
  selNode2->SetFieldType(svtkSelectionNode::CELL);
  selNode2->GetProperties()->Set(svtkSelectionNode::COMPOSITE_INDEX(), 1);
  selNode2->SetSelectionList(selArr2);
  svtkSmartPointer<svtkSelection> sel2 = svtkSmartPointer<svtkSelection>::New();
  sel2->AddNode(selNode2);

  // Create selection at distance of 2
  svtkSmartPointer<svtkCellDistanceSelector> ls2 = svtkSmartPointer<svtkCellDistanceSelector>::New();
  ls2->SetInputMesh(mesh);
  ls2->SetInputSelection(sel2);
  ls2->SetDistance(2);
  ls2->AddIntermediateOff();

  // Extract selection from mesh
  svtkSmartPointer<svtkExtractSelection> es2 = svtkSmartPointer<svtkExtractSelection>::New();
  es2->SetInputData(0, mesh);
  es2->SetInputConnection(1, ls2->GetOutputPort());
  es2->Update();
  testIntValue += CheckExtractedUGrid(es2, "Selection d({7632})=0|2", 2, true);

  // *****************************************************************************
  // 3. Selection within distance of 1 from cells 6413, 7268, and 7399
  // *****************************************************************************

  // Create a selection, sel3, of cells with indices 6413, 7268, and 7399
  svtkSmartPointer<svtkIdTypeArray> selArr3 = svtkSmartPointer<svtkIdTypeArray>::New();
  selArr3->InsertNextValue(6413);
  selArr3->InsertNextValue(7268);
  selArr3->InsertNextValue(7399);
  svtkSmartPointer<svtkSelectionNode> selNode3 = svtkSmartPointer<svtkSelectionNode>::New();
  selNode3->SetContentType(svtkSelectionNode::INDICES);
  selNode3->SetFieldType(svtkSelectionNode::CELL);
  selNode3->GetProperties()->Set(svtkSelectionNode::COMPOSITE_INDEX(), 1);
  selNode3->SetSelectionList(selArr3);
  svtkSmartPointer<svtkSelection> sel3 = svtkSmartPointer<svtkSelection>::New();
  sel3->AddNode(selNode3);

  // Create selection within distance of 1
  svtkSmartPointer<svtkCellDistanceSelector> ls3 = svtkSmartPointer<svtkCellDistanceSelector>::New();
  ls3->SetInputMesh(mesh);
  ls3->SetInputSelection(sel3);
  ls3->SetDistance(1);

  // Extract selection from mesh
  svtkSmartPointer<svtkExtractSelection> es3 = svtkSmartPointer<svtkExtractSelection>::New();
  es3->SetInputData(0, mesh);
  es3->SetInputConnection(1, ls3->GetOutputPort());
  es3->Update();
  testIntValue += CheckExtractedUGrid(es3, "Selection d({6413,7268,7399})<2", 3, true);

  return testIntValue;
}
