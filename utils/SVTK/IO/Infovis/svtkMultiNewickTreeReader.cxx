/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiNewickTreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiNewickTreeReader.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkNewickTreeReader.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtksys/FStream.hxx"

#include <fstream>
#include <iostream>

svtkStandardNewMacro(svtkMultiNewickTreeReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
svtkMultiNewickTreeReader::svtkMultiNewickTreeReader()
{
  svtkMultiPieceDataSet* output = svtkMultiPieceDataSet::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
svtkMultiNewickTreeReader::~svtkMultiNewickTreeReader() = default;

//----------------------------------------------------------------------------
svtkMultiPieceDataSet* svtkMultiNewickTreeReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkMultiPieceDataSet* svtkMultiNewickTreeReader::GetOutput(int idx)
{
  return svtkMultiPieceDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkMultiNewickTreeReader::SetOutput(svtkMultiPieceDataSet* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkMultiNewickTreeReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkDebugMacro(<< "Reading Multiple Newick trees ...");

  if (fname.empty())
  {
    svtkErrorMacro(<< "Input filename not set");
    return 1;
  }

  svtksys::ifstream ifs(fname.c_str(), svtksys::ifstream::in);
  if (!ifs.good())
  {
    svtkErrorMacro(<< "Unable to open " << fname << " for reading");
    return 1;
  }

  svtkMultiPieceDataSet* const output = svtkMultiPieceDataSet::SafeDownCast(doOutput);

  // Read the input file into a char *
  int fileSize;
  ifs.seekg(0, std::ios::end);
  fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  char* buffer = new char[fileSize + 1];
  ifs.read(buffer, fileSize);
  ifs.close();
  buffer[fileSize] = '\0';

  // use the separator ";" to decide how many trees are contained in the file
  char* current = buffer;
  unsigned int NumOfTrees = 0;
  while (*current != '\0')
  {
    while (*current == '\n' || *current == ' ')
    { // ignore extra \n and spaces
      current++;
    }

    char* currentTreeStart = current; // record the starting char of the tree
    unsigned int singleTreeLength = 0;
    while (*current != ';' && *current != '\0')
    {
      singleTreeLength++;
      current++;
    }

    if (*current == ';') // each newick tree string ends with ";"
    {
      char* singleTreeBuffer = new char[singleTreeLength + 1];
      for (unsigned int i = 0; i < singleTreeLength; i++)
      {
        singleTreeBuffer[i] = *(currentTreeStart + i);
      }
      singleTreeBuffer[singleTreeLength] = '\0';
      current++; // skip ';'

      svtkNew<svtkNewickTreeReader> treeReader;
      svtkSmartPointer<svtkTree> tree = svtkSmartPointer<svtkTree>::New();
      treeReader->ReadNewickTree(singleTreeBuffer, *tree);

      output->SetPiece(NumOfTrees, tree);
      NumOfTrees++;

      delete[] singleTreeBuffer;
    }
  }
  delete[] buffer;

  return 1;
}

//----------------------------------------------------------------------------
int svtkMultiNewickTreeReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiPieceDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkMultiNewickTreeReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
