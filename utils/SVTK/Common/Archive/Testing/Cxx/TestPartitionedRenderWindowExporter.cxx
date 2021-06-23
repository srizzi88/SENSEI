/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkConeSource.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkWindowNode.h"
#include "svtksys/SystemTools.hxx"

#include "svtkArchiver.h"
#include "svtkJSONRenderWindowExporter.h"

#include "svtkPartitionedArchiver.h"

#include <archive.h>
#include <archive_entry.h>

#include <cstdio>
#include <cstring>
#include <fstream>

// Construct a scene and write it to disk and to buffer. Decompress the buffer
// and compare its contents to the files on disk.
int TestPartitionedRenderWindowExporter(int argc, char* argv[])
{
  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string directoryName = testDirectory + std::string("/") + std::string("ExportVtkJS");

  svtkNew<svtkConeSource> cone;
  svtkNew<svtkPolyDataMapper> pmap;
  pmap->SetInputConnection(cone->GetOutputPort());

  svtkNew<svtkRenderWindow> rwin;

  svtkNew<svtkRenderer> ren;
  rwin->AddRenderer(ren);

  svtkNew<svtkLight> light;
  ren->AddLight(light);

  svtkNew<svtkActor> actor;
  ren->AddActor(actor);

  actor->SetMapper(pmap);

  {
    svtkNew<svtkJSONRenderWindowExporter> exporter;
    exporter->GetArchiver()->SetArchiveName(directoryName.c_str());
    exporter->SetRenderWindow(rwin);
    exporter->Write();
  }

  svtkNew<svtkJSONRenderWindowExporter> exporter;
  svtkNew<svtkPartitionedArchiver> partitionedArchiver;
  exporter->SetArchiver(partitionedArchiver);
  exporter->SetRenderWindow(rwin);
  exporter->Write();

  for (std::size_t i = 0; i < partitionedArchiver->GetNumberOfBuffers(); i++)
  {
    std::string bufferName(partitionedArchiver->GetBufferName(i));

    struct archive* a = archive_read_new();
    archive_read_support_filter_gzip(a);
    archive_read_support_format_zip(a);
#if ARCHIVE_VERSION_NUMBER < 3002000
    int r = archive_read_open_memory(a,
      const_cast<char*>(partitionedArchiver->GetBuffer(bufferName.c_str())),
      partitionedArchiver->GetBufferSize(bufferName.c_str()));
#else
    int r = archive_read_open_memory(a, partitionedArchiver->GetBuffer(bufferName.c_str()),
      partitionedArchiver->GetBufferSize(bufferName.c_str()));
#endif

    if (r != ARCHIVE_OK)
    {
      svtkErrorWithObjectMacro(nullptr, "Cannot open archive from memory");
      return EXIT_FAILURE;
    }

    struct archive_entry* entry;
    char* buffer;
    std::size_t size;
    if (archive_read_next_header(a, &entry) != ARCHIVE_OK || r != ARCHIVE_OK)
    {
      svtkErrorWithObjectMacro(nullptr, "Cannot access archive header");
      return EXIT_FAILURE;
    }

    {
      std::string fileName = directoryName + "/" + archive_entry_pathname(entry);
      size = archive_entry_size(entry);
      buffer = (char*)malloc(size);
      if (buffer == nullptr)
      {
        svtkErrorWithObjectMacro(nullptr, "Could not allocate buffer");
        r = ARCHIVE_FATAL;
        break;
      }

      archive_read_data(a, buffer, size);

      {
        std::FILE* fp;
        char* fbuffer;

        fp = std::fopen(fileName.c_str(), "rb");
        if (fp == nullptr)
        {
          svtkErrorWithObjectMacro(nullptr, "Could not open file on disk");
          r = ARCHIVE_FATAL;
          break;
        }

        std::fseek(fp, 0L, SEEK_END);
        long lSize = std::ftell(fp);
        if (size != static_cast<std::size_t>(lSize))
        {
          svtkErrorWithObjectMacro(nullptr, "Buffered file size does not match file size on disk");
          r = ARCHIVE_FATAL;
          std::fclose(fp);
          break;
        }

        std::rewind(fp);

        // allocate memory for entire content
        fbuffer = (char*)malloc(lSize);
        if (fbuffer == nullptr)
        {
          r = ARCHIVE_FATAL;
          std::fclose(fp);
          std::free(buffer);
          break;
        }

        // copy the file into the buffer
        std::size_t r_ = std::fread(fbuffer, lSize, 1, fp);
        (void)r_;

        if (std::memcmp(fbuffer, buffer, size) != 0)
        {
          svtkErrorWithObjectMacro(nullptr, "Buffered file does not match file on disk");
          r = ARCHIVE_FATAL;
        }

        std::fclose(fp);
        std::free(buffer);
        std::free(fbuffer);
      }
    }

    if (r != ARCHIVE_OK)
    {
      svtkErrorWithObjectMacro(nullptr, "Comparison to on-disk archive failed");
      archive_read_free(a);
      return EXIT_FAILURE;
    }

    r = archive_read_free(a);
    if (r != ARCHIVE_OK)
    {
      svtkErrorWithObjectMacro(nullptr, "Cannot close archive");
      return EXIT_FAILURE;
    }
  }

  svtksys::SystemTools::RemoveADirectory(directoryName.c_str());

  return EXIT_SUCCESS;
}
