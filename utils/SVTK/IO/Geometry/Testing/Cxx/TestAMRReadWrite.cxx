/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkSimplePointsReader and svtkSimplePointsWriter
// .SECTION Description
//
#include "svtkAMREnzoReader.h"
#include "svtkAMRInformation.h"
#include "svtkCompositeDataReader.h"
#include "svtkCompositeDataWriter.h"
#include "svtkNew.h"
#include "svtkOverlappingAMR.h"
#include "svtkSetGet.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkUniformGrid.h"

namespace
{
svtkSmartPointer<svtkOverlappingAMR> CreateTestAMR(int argc, char* argv[])
{
  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");
  svtkNew<svtkAMREnzoReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->SetMaxLevel(8);
  reader->SetCellArrayStatus("TotalEnergy", 1);
  reader->Update();
  svtkSmartPointer<svtkOverlappingAMR> ret =
    svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  return ret;
}
}

int TestAMRReadWrite(int argc, char* argv[])
{

  svtkSmartPointer<svtkOverlappingAMR> amr = CreateTestAMR(argc, argv);
  svtkNew<svtkCompositeDataWriter> writer;
  writer->SetInputData(amr);
  writer->SetFileName("testamr");
  writer->Update();

  svtkNew<svtkCompositeDataReader> reader;
  reader->SetFileName("testamr");
  reader->Update();

  svtkSmartPointer<svtkOverlappingAMR> amr1 =
    svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));

  int errors(0);
  errors += !(*amr1->GetAMRInfo() == *amr->GetAMRInfo());

  return errors;
}
