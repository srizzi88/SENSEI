/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImportExport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkImageExport.h"
#include "svtkImageImport.h"
#include "svtkImageReader.h"
#include "svtkImageViewer.h"
#include "svtkPNMWriter.h"
#include "svtkWindowToImageFilter.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int ImportExport(int argc, char* argv[])
{
  int i, j, k;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkSmartPointer<svtkImageReader> reader = svtkSmartPointer<svtkImageReader>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetFilePrefix(fname);
  reader->SetDataMask(0x7fff);
  delete[] fname;

  // create exporter
  svtkSmartPointer<svtkImageExport> exporter = svtkSmartPointer<svtkImageExport>::New();
  exporter->SetInputConnection(reader->GetOutputPort());
  exporter->ImageLowerLeftOn();

  // get info from exporter and create array to hold data
  int memsize = exporter->GetDataMemorySize();
  int* dimensions = exporter->GetDataDimensions();

  // export the data into the array
  short* data = new short[memsize / sizeof(short)];
  exporter->Export(data);

  // alternative method for getting data
  // short *data = exporter->GetPointerToData();

  // do a little something to the data

  for (i = 0; i < dimensions[2]; i++)
  {
    for (j = 0; j < dimensions[1]; j++)
    {
      for (k = 0; k < dimensions[0]; k++)
      {
        if (k % 10 == 0)
        {
          data[k + dimensions[0] * (j + dimensions[1] * i)] = 0;
        }
        if (j % 10 == 0)
        {
          data[k + dimensions[0] * (j + dimensions[1] * i)] = 1000;
        }
      }
    }
  }

  // create an importer to read the data back in
  svtkSmartPointer<svtkImageImport> importer = svtkSmartPointer<svtkImageImport>::New();
  importer->SetWholeExtent(1, dimensions[0], 1, dimensions[1], 1, dimensions[2]);
  importer->SetDataExtentToWholeExtent();
  importer->SetDataScalarTypeToShort();
  importer->SetImportVoidPointer(data);
  importer->SetScalarArrayName("importedScalars");

  svtkSmartPointer<svtkImageViewer> viewer = svtkSmartPointer<svtkImageViewer>::New();
  viewer->SetInputConnection(importer->GetOutputPort());
  viewer->SetZSlice(45);
  viewer->SetColorWindow(2000);
  viewer->SetColorLevel(1000);

  viewer->Render();

  int retVal = svtkRegressionTestImage(viewer->GetRenderWindow());

  delete[] data;

  return !retVal;
}
