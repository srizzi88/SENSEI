/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinearSelector2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExtractSelection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLinearSelector.h"
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

#if 0
// Reference value
const svtkIdType cardSelectionLinearSelector2D  = 20;

// ------------------------------------------------------------------------------------------------
static int CheckExtractedUGrid( svtkExtractSelection* extract,
                                const char* tag,
                                int testIdx,
                                bool writeGrid )
{
  // Output must be a multiblock dataset
  svtkMultiBlockDataSet* outputMB = svtkMultiBlockDataSet::SafeDownCast( extract->GetOutput() );
  if ( ! outputMB )
  {
    svtkGenericWarningMacro("Cannot downcast extracted selection to multiblock dataset.");

    return 1;
  }

  // First block must be an unstructured grid
  svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast( outputMB->GetBlock( 0 ) );
  if ( ! ugrid )
  {
    svtkGenericWarningMacro("Cannot downcast extracted selection to unstructured grid.");

    return 1;
  }

  // Initialize test status
  int testStatus = 0;
  cerr << endl;

  // Verify selection cardinality
  svtkIdType nCells = ugrid->GetNumberOfCells();
  cout << tag
       << " contains "
       << nCells
       << " cells."
       << endl;

  if ( nCells != cardSelectionLinearSelector2D )
  {
    svtkGenericWarningMacro( "Incorrect cardinality: "
                           << nCells
                           << " != "
                           <<  cardSelectionLinearSelector2D );
    testStatus = 1;
  }

  // Verify selection cells
  cerr << "Original cell Ids (types): ";
  ugrid->GetCellData()->SetActiveScalars( "svtkOriginalCellIds" );
  svtkDataArray* oCellIds = ugrid->GetCellData()->GetScalars();
  for ( svtkIdType i = 0; i < oCellIds->GetNumberOfTuples(); ++ i )
  {
    cerr << oCellIds->GetTuple1( i )
         << " ";
  }
  cerr << endl;

  // If requested, write mesh
  if ( writeGrid )
  {
    std::ostringstream fileNameSS;
    fileNameSS << "./LinearExtraction2D-"
               << testIdx
               << ".svtk";
    svtkSmartPointer<svtkUnstructuredGridWriter> writer = svtkSmartPointer<svtkUnstructuredGridWriter>::New();
    writer->SetFileName( fileNameSS.str().c_str() );
    writer->SetInputData( ugrid );
    writer->Write();
    cerr << "Wrote file "
         << fileNameSS.str()
         << endl;
  }

  return testStatus;
}
#endif

//----------------------------------------------------------------------------
int TestLinearSelector2D(int argc, char* argv[])
{
  // Initialize test value
  int testIntValue = 0;

  // Read 2D unstructured input mesh
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SemiDisk/SemiDisk.svtk");
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
  // Selection along inner segment with endpoints (35.84,0,0) and (36.9,0.03,0)
  // *****************************************************************************

  // Create selection along one line segment
  svtkSmartPointer<svtkLinearSelector> ls = svtkSmartPointer<svtkLinearSelector>::New();
  ls->SetInputData(mesh);
  ls->SetStartPoint(35.84, .0, .0);
  ls->SetEndPoint(36.9, .03, .0);
  ls->IncludeVerticesOff();
  ls->SetVertexEliminationTolerance(1.e-12);

  // Extract selection from mesh
  svtkSmartPointer<svtkExtractSelection> es = svtkSmartPointer<svtkExtractSelection>::New();
  es->SetInputData(0, mesh);
  es->SetInputConnection(1, ls->GetOutputPort());
  es->Update();

#if 0
  testIntValue += CheckExtractedUGrid( es, "Selection (35.84,0,0)-(36.9,0.03,0)", 0, true );
#endif

  return testIntValue;
}
