/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOMETIFFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkOMETIFFReader
 * @brief reader for OME TIFF files
 *
 * svtkOMETIFFReader supports reading OME-TIFF files. These are TIFF files with
 * OME meta-data that helps interpret the contents of the TIFF.
 *
 * The OME-TIFF specification is available here:
 * https://docs.openmicroscopy.org/ome-model/5.6.3/ome-tiff/specification.html#ome-tiff-specification
 *
 * The current implementation only supports single-file, multi-page TIFF. It
 * will not read multi-file OME-TIFF files correctly.
 *
 * Unlike most image readers, this reader does not support arbitrary sub-extent
 * requests. This is because the splicing of the `z`, `t`, and `c` planes can make it
 * tricky to read sub-extents in `z` for certain dimension orders. This reader
 * supports piece-request instead and satisfies such request by splitting the
 * `XY` plane into requested number of pieces.
 *
 * The reader lets the superclass read the whole TIFF volume and then splice it
 * up into channels, timesteps, and z-planes. The parts are then cached
 * internally so that subsequent timestep requests can be served without
 * re-reading the file.
 */

#ifndef svtkOMETIFFReader_h
#define svtkOMETIFFReader_h

#include "svtkTIFFReader.h"

class SVTKIOIMAGE_EXPORT svtkOMETIFFReader : public svtkTIFFReader
{
public:
  static svtkOMETIFFReader* New();
  svtkTypeMacro(svtkOMETIFFReader, svtkTIFFReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  int CanReadFile(const char* fname) override;
  const char* GetFileExtensions() override { return ".ome.tif .ome.tiff"; }
  const char* GetDescriptiveName() override { return "OME TIFF"; }
  //@}

protected:
  svtkOMETIFFReader();
  ~svtkOMETIFFReader() override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;

private:
  svtkOMETIFFReader(const svtkOMETIFFReader&) = delete;
  void operator=(const svtkOMETIFFReader&) = delete;

  class svtkOMEInternals;
  svtkOMEInternals* OMEInternals;
};

#endif
