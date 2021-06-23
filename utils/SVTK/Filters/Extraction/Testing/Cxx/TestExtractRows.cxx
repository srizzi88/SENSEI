/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractRows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkExtractSelection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkTable.h"

#define ROWS 15
#define COLUMNS 4

int TestExtractRows(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  svtkNew<svtkTable> table;

  const char* name[] = { "foo", "bar", "baz", "foobar" };

  svtkIdType counter = 0;
  for (int i = 0; i < COLUMNS; ++i)
  {
    svtkNew<svtkIdTypeArray> col;
    col->SetName(name[i]);
    for (int j = 0; j < ROWS; ++j)
    {
      col->InsertNextValue(counter);
      ++counter;
    }
    table->AddColumn(col);
  }

  svtkNew<svtkExtractSelection> extractionFilter;
  extractionFilter->PreserveTopologyOff();
  svtkNew<svtkSelection> selection;
  svtkNew<svtkSelectionNode> node;

  node->Initialize();
  node->GetProperties()->Set(svtkSelectionNode::CONTENT_TYPE(), svtkSelectionNode::VALUES);
  node->SetFieldType(svtkSelectionNode::ROW);
  svtkNew<svtkIdTypeArray> rowIds;
  rowIds->SetNumberOfComponents(1);
  rowIds->SetNumberOfTuples(5);
  rowIds->SetName("foo");
  rowIds->SetTuple1(0, 2);
  rowIds->SetTuple1(1, 6);
  rowIds->SetTuple1(2, 9);
  rowIds->SetTuple1(3, 10);
  rowIds->SetTuple1(4, 11);
  node->SetSelectionList(rowIds);
  selection->AddNode(node);

  extractionFilter->SetInputData(0, table);
  extractionFilter->SetInputData(1, selection);
  extractionFilter->Update();

  svtkTable* output = svtkTable::SafeDownCast(extractionFilter->GetOutput());
  if (!output)
  {
    std::cerr << "Extracting rows did not produce a table." << std::endl;
    return 1;
  }
  if (output->GetNumberOfRows() != 5)
  {
    std::cerr << "Result had wrong number of rows." << std::endl;
    std::cerr << "It has " << output->GetNumberOfRows() << " but should have 5." << std::endl;
    return 1;
  }
  svtkIdTypeArray* col1 = svtkIdTypeArray::SafeDownCast(output->GetColumnByName("svtkOriginalRowIds"));
  for (int i = 0; i < col1->GetNumberOfTuples(); ++i)
  {
    if (col1->GetValue(i) != rowIds->GetValue(i))
    {
      std::cerr << "Result has wrong original row id" << std::endl;
      return 1;
    }
  }
  return 0;
}
