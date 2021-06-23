/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBYUWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBYUWriter
 * @brief   write MOVIE.BYU files
 *
 * svtkBYUWriter writes MOVIE.BYU polygonal files. These files consist
 * of a geometry file (.g), a scalar file (.s), a displacement or
 * vector file (.d), and a 2D texture coordinate file (.t). These files
 * must be specified to the object, the appropriate boolean
 * variables must be true, and data must be available from the input
 * for the files to be written.
 * WARNING: this writer does not currently write triangle strips. Use
 * svtkTriangleFilter to convert strips to triangles.
 */

#ifndef svtkBYUWriter_h
#define svtkBYUWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkWriter.h"

class svtkPolyData;

class SVTKIOGEOMETRY_EXPORT svtkBYUWriter : public svtkWriter
{
public:
  static svtkBYUWriter* New();

  svtkTypeMacro(svtkBYUWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the geometry file to write.
   */
  svtkSetStringMacro(GeometryFileName);
  svtkGetStringMacro(GeometryFileName);
  //@}

  //@{
  /**
   * Specify the name of the displacement file to write.
   */
  svtkSetStringMacro(DisplacementFileName);
  svtkGetStringMacro(DisplacementFileName);
  //@}

  //@{
  /**
   * Specify the name of the scalar file to write.
   */
  svtkSetStringMacro(ScalarFileName);
  svtkGetStringMacro(ScalarFileName);
  //@}

  //@{
  /**
   * Specify the name of the texture file to write.
   */
  svtkSetStringMacro(TextureFileName);
  svtkGetStringMacro(TextureFileName);
  //@}

  //@{
  /**
   * Turn on/off writing the displacement file.
   */
  svtkSetMacro(WriteDisplacement, svtkTypeBool);
  svtkGetMacro(WriteDisplacement, svtkTypeBool);
  svtkBooleanMacro(WriteDisplacement, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off writing the scalar file.
   */
  svtkSetMacro(WriteScalar, svtkTypeBool);
  svtkGetMacro(WriteScalar, svtkTypeBool);
  svtkBooleanMacro(WriteScalar, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off writing the texture file.
   */
  svtkSetMacro(WriteTexture, svtkTypeBool);
  svtkGetMacro(WriteTexture, svtkTypeBool);
  svtkBooleanMacro(WriteTexture, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  svtkPolyData* GetInput();
  svtkPolyData* GetInput(int port);
  //@}

protected:
  svtkBYUWriter();
  ~svtkBYUWriter() override;

  void WriteData() override;

  char* GeometryFileName;
  char* DisplacementFileName;
  char* ScalarFileName;
  char* TextureFileName;
  svtkTypeBool WriteDisplacement;
  svtkTypeBool WriteScalar;
  svtkTypeBool WriteTexture;

  void WriteGeometryFile(FILE* fp, int numPts);
  void WriteDisplacementFile(int numPts);
  void WriteScalarFile(int numPts);
  void WriteTextureFile(int numPts);

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkBYUWriter(const svtkBYUWriter&) = delete;
  void operator=(const svtkBYUWriter&) = delete;
};

#endif
