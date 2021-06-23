/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMetaIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkMetaIO / MetaImage
// .SECTION Description
//

#include "svtkDataObject.h"
#include "svtkImageData.h"
#include "svtkImageMathematics.h"
#include "svtkMetaImageReader.h"
#include "svtkMetaImageWriter.h"
#include "svtkObjectFactory.h"
#include "svtkOutputWindow.h"

int TestMetaIO(int argc, char* argv[])
{
  svtkOutputWindow::GetInstance()->PromptUserOn();
  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <meta image file>" << endl;
    return 1;
  }

  svtkMetaImageReader* reader = svtkMetaImageReader::New();
  reader->SetFileName(argv[1]);
  reader->Update();
  cout << "10, 10, 10 : (1) : " << reader->GetOutput()->GetScalarComponentAsFloat(10, 10, 10, 0)
       << endl;
  cout << "24, 37, 10 : (168) : " << reader->GetOutput()->GetScalarComponentAsFloat(24, 37, 10, 0)
       << endl;

  svtkMetaImageWriter* writer = svtkMetaImageWriter::New();
  writer->SetFileName("TestMetaIO.mha");
  writer->SetInputConnection(reader->GetOutputPort());
  writer->Write();

  reader->Delete();
  writer->Delete();

  svtkMetaImageReader* readerStd = svtkMetaImageReader::New();
  readerStd->SetFileName(argv[1]);
  readerStd->Update();

  svtkMetaImageReader* readerNew = svtkMetaImageReader::New();
  readerNew->SetFileName("TestMetaIO.mha");
  readerNew->Update();

  double error = 0;
  int* ext = readerStd->GetOutput()->GetExtent();
  for (int z = ext[4]; z <= ext[5]; z += 2)
  {
    for (int y = ext[2]; y <= ext[3]; y++)
    {
      for (int x = ext[0]; x <= ext[1]; x++)
      {
        error += fabs(readerStd->GetOutput()->GetScalarComponentAsFloat(x, y, z, 0) -
          readerNew->GetOutput()->GetScalarComponentAsFloat(x, y, z, 0));
      }
    }
  }

  readerStd->Delete();
  readerNew->Delete();

  if (error > 1)
  {
    cerr << "Error: Image difference on read/write = " << error << endl;
    return 1;
  }

  cout << "Success!  Error = " << error << endl;

  return 0;
}
