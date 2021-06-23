/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredGridPartitioner.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestStructuredGridPartitioner.cxx -- Simple test for partitioning
//  structured grids.
//
// .SECTION Description
//  Simple test for structured grid partitioner

#include <cassert>
#include <iostream>
#include <sstream>

#include "svtkMultiBlockDataSet.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridPartitioner.h"
#include "svtkXMLMultiBlockDataWriter.h"
#include "svtkXMLStructuredGridReader.h"

//------------------------------------------------------------------------------
// Description:
// Writes multi-block dataset to grid
void WriteMultiBlock(const std::string& file, svtkMultiBlockDataSet* mbds)
{
  assert("pre: nullptr multi-block dataset!" && (mbds != nullptr));

  std::ostringstream oss;
  svtkXMLMultiBlockDataWriter* writer = svtkXMLMultiBlockDataWriter::New();

  oss << file << "." << writer->GetDefaultFileExtension();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(mbds);
  writer->Update();
  writer->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Get grid from file
svtkStructuredGrid* GetGridFromFile(std::string& file)
{
  svtkXMLStructuredGridReader* reader = svtkXMLStructuredGridReader::New();
  reader->SetFileName(file.c_str());
  reader->Update();
  svtkStructuredGrid* myGrid = svtkStructuredGrid::New();
  myGrid->DeepCopy(reader->GetOutput());
  reader->Delete();
  return (myGrid);
}

//------------------------------------------------------------------------------
// Description:
// Program Main
int TestStructuredGridPartitioner(int argc, char* argv[])
{

  if (argc != 3)
  {
    std::cout << "Usage: ./TestStructuredGridPartitioner <vtsfile> <N>\n";
    std::cout.flush();
    return -1;
  }

  std::string fileName = std::string(argv[1]);
  int NumPartitions = atoi(argv[2]);

  svtkStructuredGrid* grid = GetGridFromFile(fileName);
  assert("pre: grid is not nullptr" && (grid != nullptr));

  svtkStructuredGridPartitioner* gridPartitioner = svtkStructuredGridPartitioner::New();
  gridPartitioner->SetInputData(grid);
  gridPartitioner->SetNumberOfPartitions(NumPartitions);
  gridPartitioner->Update();

  svtkMultiBlockDataSet* mbds = gridPartitioner->GetOutput();
  WriteMultiBlock("PartitionedGrid", mbds);

  grid->Delete();
  gridPartitioner->Delete();
  return 0;
}
