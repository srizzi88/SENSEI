/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMCubesWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMCubesWriter
 * @brief   write binary marching cubes file
 *
 * svtkMCubesWriter is a polydata writer that writes binary marching cubes
 * files. (Marching cubes is an isosurfacing technique that generates many
 * triangles.) The binary format is supported by W. Lorensen's marching cubes
 * program (and the svtkSliceCubes object). Each triangle is represented by
 * three records, with each record consisting of six single precision
 * floating point numbers representing the a triangle vertex coordinate and
 * vertex normal.
 *
 * @warning
 * Binary files are written in sun/hp/sgi (i.e., Big Endian) form.
 *
 * @sa
 * svtkMarchingCubes svtkSliceCubes svtkMCubesReader
 */

#ifndef svtkMCubesWriter_h
#define svtkMCubesWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkWriter.h"

class svtkCellArray;
class svtkDataArray;
class svtkPoints;
class svtkPolyData;

class SVTKIOGEOMETRY_EXPORT svtkMCubesWriter : public svtkWriter
{
public:
  static svtkMCubesWriter* New();
  svtkTypeMacro(svtkMCubesWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get file name of marching cubes limits file.
   */
  svtkSetStringMacro(LimitsFileName);
  svtkGetStringMacro(LimitsFileName);
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  svtkPolyData* GetInput();
  svtkPolyData* GetInput(int port);
  //@}

  //@{
  /**
   * Specify file name of svtk polygon data file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkMCubesWriter();
  ~svtkMCubesWriter() override;

  void WriteData() override;

  void WriteMCubes(FILE* fp, svtkPoints* pts, svtkDataArray* normals, svtkCellArray* polys);
  void WriteLimits(FILE* fp, double* bounds);

  char* LimitsFileName;

  char* FileName;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkMCubesWriter(const svtkMCubesWriter&) = delete;
  void operator=(const svtkMCubesWriter&) = delete;
};

#endif
