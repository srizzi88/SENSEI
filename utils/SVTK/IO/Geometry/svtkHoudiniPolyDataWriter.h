/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHoudiniPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHoudiniPolyDataWriter
 * @brief   write svtk polygonal data to Houdini file.
 *
 *
 * svtkHoudiniPolyDataWriter is a source object that writes SVTK polygonal data
 * files in ASCII Houdini format (see
 * http://www.sidefx.com/docs/houdini15.0/io/formats/geo).
 */

#ifndef svtkHoudiniPolyDataWriter_h
#define svtkHoudiniPolyDataWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkWriter.h"

class svtkPolyData;

class SVTKIOGEOMETRY_EXPORT svtkHoudiniPolyDataWriter : public svtkWriter
{
public:
  static svtkHoudiniPolyDataWriter* New();
  svtkTypeMacro(svtkHoudiniPolyDataWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specifies the delimited text file to be loaded.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

protected:
  svtkHoudiniPolyDataWriter();
  ~svtkHoudiniPolyDataWriter() override;

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  char* FileName;

private:
  svtkHoudiniPolyDataWriter(const svtkHoudiniPolyDataWriter&) = delete;
  void operator=(const svtkHoudiniPolyDataWriter&) = delete;
};

#endif
