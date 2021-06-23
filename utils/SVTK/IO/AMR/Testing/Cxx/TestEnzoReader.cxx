/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEnzoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <iostream>
#include <string>

#include "svtkAMREnzoReader.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkOverlappingAMR.h"
#include "svtkSetGet.h"
#include "svtkTestUtilities.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridAMRDataIterator.h"
namespace EnzoReaderTest
{

//------------------------------------------------------------------------------
template <class T>
int CheckValue(const std::string& name, T actualValue, T expectedValue)
{
  if (actualValue != expectedValue)
  {
    std::cerr << "ERROR: " << name << " value mismatch! ";
    std::cerr << "Expected: " << expectedValue << " Actual: " << actualValue;
    std::cerr << std::endl;
    return 1;
  }
  return 0;
}

} // END namespace

int ComputeMaxNonEmptyLevel(svtkOverlappingAMR* amr)
{
  svtkUniformGridAMRDataIterator* iter =
    svtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator());
  iter->SetSkipEmptyNodes(true);
  int maxLevel(-1);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    int level = iter->GetCurrentLevel();
    if (level > maxLevel)
    {
      maxLevel = level;
    }
  }
  iter->Delete();
  return maxLevel + 1;
}

int ComputeNumberOfVisibleCells(svtkOverlappingAMR* amr)
{
  int numVisibleCells(0);
  svtkCompositeDataIterator* iter = amr->NewIterator();
  iter->SkipEmptyNodesOn();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    svtkIdType num = grid->GetNumberOfCells();
    for (svtkIdType i = 0; i < num; i++)
    {
      if (grid->IsCellVisible(i))
      {
        numVisibleCells++;
      }
    }
  }
  iter->Delete();
  return numVisibleCells;
}

int TestEnzoReader(int argc, char* argv[])
{
  int rc = 0;
  int NumBlocksPerLevel[] = { 1, 3, 1, 1, 1, 1, 1, 1 };
  int numVisibleCells[] = { 4096, 6406, 13406, 20406, 23990, 25502, 26377, 27077 };
  svtkAMREnzoReader* myEnzoReader = svtkAMREnzoReader::New();
  char* fileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");
  std::cout << "Filename: " << fileName << std::endl;
  std::cout.flush();

  svtkOverlappingAMR* amr = nullptr;
  myEnzoReader->SetFileName(fileName);
  for (int level = 0; level < myEnzoReader->GetNumberOfLevels(); ++level)
  {
    myEnzoReader->SetMaxLevel(level);
    myEnzoReader->Update();
    rc += EnzoReaderTest::CheckValue("LEVEL", myEnzoReader->GetNumberOfLevels(), 8);
    rc += EnzoReaderTest::CheckValue("BLOCKS", myEnzoReader->GetNumberOfBlocks(), 10);

    amr = myEnzoReader->GetOutput();
    amr->Audit();
    if (amr != nullptr)
    {
      rc += EnzoReaderTest::CheckValue(
        "OUTPUT LEVELS", static_cast<int>(ComputeMaxNonEmptyLevel(amr)), level + 1);
      rc += EnzoReaderTest::CheckValue("NUMBER OF BLOCKS AT LEVEL",
        static_cast<int>(amr->GetNumberOfDataSets(level)), NumBlocksPerLevel[level]);
      rc += EnzoReaderTest::CheckValue(
        "Number of Visible cells ", ComputeNumberOfVisibleCells(amr), numVisibleCells[level]);
    }
    else
    {
      std::cerr << "ERROR: output AMR dataset is nullptr!";
      return 1;
    }
  } // END for all levels

  myEnzoReader->Delete();
  delete[] fileName;
  return (rc);
}
