/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPVWebGLExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPVWebGLExporter.h"

#include "svtkBase64Utilities.h"
#include "svtkCamera.h"
#include "svtkExporter.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkWebGLExporter.h"
#include "svtkWebGLObject.h"

#include <fstream>
#include <sstream>
#include <string>
#include <svtksys/FStream.hxx>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkPVWebGLExporter);
// ---------------------------------------------------------------------------
svtkPVWebGLExporter::svtkPVWebGLExporter()
{
  this->FileName = nullptr;
}

// ---------------------------------------------------------------------------
svtkPVWebGLExporter::~svtkPVWebGLExporter()
{
  this->SetFileName(nullptr);
}

// ---------------------------------------------------------------------------
void svtkPVWebGLExporter::WriteData()
{
  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    svtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  svtkNew<svtkWebGLExporter> exporter;
  exporter->SetMaxAllowedSize(65000);

  // We use the camera focal point to be the center of rotation
  double centerOfRotation[3];
  svtkRenderer* ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  svtkCamera* cam = ren->GetActiveCamera();
  cam->GetFocalPoint(centerOfRotation);
  exporter->SetCenterOfRotation(static_cast<float>(centerOfRotation[0]),
    static_cast<float>(centerOfRotation[1]), static_cast<float>(centerOfRotation[2]));

  exporter->parseScene(this->RenderWindow->GetRenderers(), "1", SVTK_PARSEALL);

  // Write meta-data file
  std::string baseFileName = this->FileName;
  baseFileName.erase(baseFileName.size() - 6, 6);
  std::string metadatFile = this->FileName;
  FILE* fp = svtksys::SystemTools::Fopen(metadatFile, "w");
  if (!fp)
  {
    svtkErrorMacro(<< "unable to open JSON MetaData file " << metadatFile.c_str());
    return;
  }
  fputs(exporter->GenerateMetadata(), fp);
  fclose(fp);

  // Write binary objects
  svtkNew<svtkBase64Utilities> base64;
  int nbObjects = exporter->GetNumberOfObjects();
  for (int idx = 0; idx < nbObjects; ++idx)
  {
    svtkWebGLObject* obj = exporter->GetWebGLObject(idx);
    if (obj->isVisible())
    {
      int nbParts = obj->GetNumberOfParts();
      for (int part = 0; part < nbParts; ++part)
      {
        // Manage binary content
        std::stringstream filePath;
        filePath << baseFileName.c_str() << "_" << obj->GetMD5().c_str() << "_" << part;
        svtksys::ofstream binaryFile;
        binaryFile.open(filePath.str().c_str(), std::ios_base::out | std::ios_base::binary);
        binaryFile.write((const char*)obj->GetBinaryData(part), obj->GetBinarySize(part));
        binaryFile.close();

        // Manage Base64
        std::stringstream filePathBase64;
        filePathBase64 << baseFileName.c_str() << "_" << obj->GetMD5().c_str() << "_" << part
                       << ".base64";
        svtksys::ofstream base64File;
        unsigned char* output = new unsigned char[obj->GetBinarySize(part) * 2];
        int size =
          base64->Encode(obj->GetBinaryData(part), obj->GetBinarySize(part), output, false);
        base64File.open(filePathBase64.str().c_str(), std::ios_base::out);
        base64File.write((const char*)output, size);
        base64File.close();
        delete[] output;
      }
    }
  }

  // Write HTML file
  std::string htmlFile = baseFileName;
  htmlFile += ".html";
  exporter->exportStaticScene(this->RenderWindow->GetRenderers(), 300, 300, htmlFile);
}
// ---------------------------------------------------------------------------
void svtkPVWebGLExporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}
