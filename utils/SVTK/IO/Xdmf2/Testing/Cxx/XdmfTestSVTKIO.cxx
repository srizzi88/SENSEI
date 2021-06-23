/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XdmfTestSVTKIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// This tests svtkXdmfWriter and svtkXdmfReader
// It manufactures/reads a bunch of test data objects, writes them to disk
// via the xdmfwriter, reads the files back in with the xdmfreader, and
// compares the output of the reader against the input to the reader. If
// differences are found it fails and stops immediately, leaving any failed
// files around for inspection. Otherwise it deletes the temporary files it
// creates and returns success.

#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkDataObjectGenerator.h"
#include "svtkDataSet.h"
#include "svtkDataSetReader.h"
#include "svtkDataSetWriter.h"
#include "svtkPointData.h"
#include "svtkTimeSourceExample.h"
#include "svtkXdmfReader.h"
#include "svtkXdmfWriter.h"
#include "svtksys/SystemTools.hxx"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#define NUMTESTS 20

const char testobject[NUMTESTS][40] = {
  "ID1",
  "ID2",
  "UF1",
  "RG1",
  "SG1",
  "PD1",
  "PD2",
  "UG1",
  "UG2",
  "UG3",
  "UG4",
  "MB{}",
  "MB{ID1}",
  "MB{UF1}",
  "MB{RG1}",
  "MB{SG1}",
  "MB{PD1}",
  "MB{UG1}",
  "MB{ ID1 UF1 RG1 SG1 PD1 UG1 }",
  "HB[ (UF1)(UF1)(UF1) ]",
};

bool CleanUpGood = true;

bool DoFilesExist(const char* xdmffile, const char* hdf5file, bool deleteIfSo)
{
  bool xexists = (xdmffile ? svtksys::SystemTools::FileExists(xdmffile) : true);
  bool hexists = (hdf5file ? svtksys::SystemTools::FileExists(hdf5file) : true);
  bool xlenOK = (xdmffile ? svtksys::SystemTools::FileLength(xdmffile) != 0 : true);
  bool hlenOK = (hdf5file ? svtksys::SystemTools::FileLength(hdf5file) != 0 : true);

  bool theyDo = xexists && xlenOK && hexists && hlenOK;
  if (theyDo && deleteIfSo && CleanUpGood)
  {
    if (xdmffile)
    {
      unlink(xdmffile);
    }
    if (hdf5file)
    {
      unlink(hdf5file);
    }
  }

  return theyDo;
}

bool DoDataObjectsDiffer(svtkDataObject* dobj1, svtkDataObject* dobj2)
{
  if (strcmp(dobj1->GetClassName(), dobj2->GetClassName()))
  {
    cerr << "Class name test failed " << dobj1->GetClassName() << " != " << dobj2->GetClassName()
         << endl;
    // return true; //disable for now
  }
  if (dobj1->GetFieldData()->GetNumberOfArrays() != dobj2->GetFieldData()->GetNumberOfArrays())
  {
    cerr << "Number of field arrays test failed" << endl;
    return true;
  }
  if (!dobj1->IsA("svtkPolyData") && !dobj1->IsA("svtkMultiBlockDataSet") &&
    dobj1->GetActualMemorySize() != dobj2->GetActualMemorySize())
  {
    cerr << "Mem size test failed" << endl;
    return true;
  }
  svtkDataSet* ds1 = svtkDataSet::SafeDownCast(dobj1);
  svtkDataSet* ds2 = svtkDataSet::SafeDownCast(dobj2);
  if (ds1 && ds2)
  {
    if ((ds1->GetNumberOfCells() != ds2->GetNumberOfCells()) ||
      (ds1->GetNumberOfPoints() != ds2->GetNumberOfPoints()))
    {
      cerr << "Number of Cells/Points test failed" << endl;
      return true;
    }
    const double* bds1 = ds1->GetBounds();
    const double* bds2 = ds2->GetBounds();
    if ((bds1[0] != bds2[0]) || (bds1[1] != bds2[1]) || (bds1[2] != bds2[2]) ||
      (bds1[3] != bds2[3]) || (bds1[4] != bds2[4]) || (bds1[5] != bds2[5]))
    {
      cerr << "Bounds test failed" << endl;
      return true;
    }
    if ((ds1->GetPointData()->GetNumberOfArrays() != ds2->GetPointData()->GetNumberOfArrays()) ||
      (ds1->GetCellData()->GetNumberOfArrays() != ds2->GetCellData()->GetNumberOfArrays()))
    {
      cerr << "Number of data arrays test failed" << endl;
      return true;
    }
    // TODO:Check array names, types, widths and ranges
  }
  return false;
}

bool TestXDMFConversion(svtkDataObject* input, char* prefix)
{
  char xdmffile[SVTK_MAXPATH];
  char hdf5file[SVTK_MAXPATH];
  char svtkfile[SVTK_MAXPATH];
  snprintf(xdmffile, sizeof(xdmffile), "%s.xmf", prefix);
  snprintf(hdf5file, sizeof(hdf5file), "%s.h5", prefix);
  snprintf(svtkfile, sizeof(svtkfile), "%s.svtk", prefix);

  svtkXdmfWriter* xwriter = svtkXdmfWriter::New();
  xwriter->SetLightDataLimit(10000);
  xwriter->WriteAllTimeStepsOn();
  xwriter->SetFileName(xdmffile);
  xwriter->SetInputData(input);
  xwriter->Write();

  xwriter->Delete();
  svtkDataSet* ds = svtkDataSet::SafeDownCast(input);
  if (ds)
  {
    svtkDataSetWriter* dsw = svtkDataSetWriter::New();
    dsw->SetFileName(svtkfile);
    dsw->SetInputData(ds);
    dsw->Write();
    dsw->Delete();
  }

  if (!DoFilesExist(xdmffile, nullptr, false))
  {
    cerr << "Writer did not create " << xdmffile << endl;
    return true;
  }

  // TODO: Once it works, enable this
  svtkXdmfReader* xreader = svtkXdmfReader::New();
  xreader->SetFileName(xdmffile);
  xreader->Update();
  svtkDataObject* rOutput = xreader->GetOutputDataObject(0);

  bool fail = DoDataObjectsDiffer(input, rOutput);
  if (!fail && CleanUpGood)
  {
    // test passed!
    unlink(xdmffile);
    unlink(hdf5file);
    unlink(svtkfile);
  }

  xreader->Delete();
  return fail;
}

int XdmfTestSVTKIO(int ac, char* av[])
{

  for (int i = 1; i < ac; i++)
  {
    if (!strcmp(av[i], "--dont-clean"))
    {
      CleanUpGood = false;
    }
  }

  bool fail = false;

  // TEST SET 1
  svtkDataObjectGenerator* dog = svtkDataObjectGenerator::New();
  int i = 0;
  while (!fail && i < NUMTESTS)
  {
    char filename[SVTK_MAXPATH];
    snprintf(filename, sizeof(filename), "xdmfIOtest_%d", i);
    cerr << "Test svtk object " << testobject[i] << endl;
    dog->SetProgram(testobject[i]);
    dog->Update();
    fail = TestXDMFConversion(dog->GetOutput(), filename);
    i++;
  }

  dog->Delete();

  if (fail)
  {
    return SVTK_ERROR;
  }

  // TEST SET 2
  cerr << "Test temporal data" << endl;
  svtkTimeSourceExample* tsrc = svtkTimeSourceExample::New();
  tsrc->GrowingOn();
  tsrc->SetXAmplitude(2.0);

  svtkXdmfWriter* xwriter = svtkXdmfWriter::New();
  xwriter->SetLightDataLimit(10000);
  xwriter->WriteAllTimeStepsOn();
  xwriter->SetFileName("xdmfIOtest_temporal_1.xmf");
  xwriter->SetInputConnection(0, tsrc->GetOutputPort(0));
  xwriter->Write();

  xwriter->Delete();
  tsrc->Delete();

  fail = !DoFilesExist("xdmfIOtest_temporal_1.xmf", nullptr, true);
  if (fail)
  {
    cerr << "Failed Temporal Test 1" << endl;
    return SVTK_ERROR;
  }

  // ETC.
  return 0;
}
