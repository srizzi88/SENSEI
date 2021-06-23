/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBYUReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBYUReader
 * @brief   read MOVIE.BYU polygon files
 *
 * svtkBYUReader is a source object that reads MOVIE.BYU polygon files.
 * These files consist of a geometry file (.g), a scalar file (.s), a
 * displacement or vector file (.d), and a 2D texture coordinate file
 * (.t).
 */

#ifndef svtkBYUReader_h
#define svtkBYUReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKIOGEOMETRY_EXPORT svtkBYUReader : public svtkPolyDataAlgorithm
{
public:
  static svtkBYUReader* New();

  svtkTypeMacro(svtkBYUReader, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify name of geometry FileName.
   */
  svtkSetStringMacro(GeometryFileName);
  svtkGetStringMacro(GeometryFileName);
  //@}

  /**
   * Specify name of geometry FileName (alias).
   */
  virtual void SetFileName(const char* f) { this->SetGeometryFileName(f); }
  virtual char* GetFileName() { return this->GetGeometryFileName(); }

  //@{
  /**
   * Specify name of displacement FileName.
   */
  svtkSetStringMacro(DisplacementFileName);
  svtkGetStringMacro(DisplacementFileName);
  //@}

  //@{
  /**
   * Specify name of scalar FileName.
   */
  svtkSetStringMacro(ScalarFileName);
  svtkGetStringMacro(ScalarFileName);
  //@}

  //@{
  /**
   * Specify name of texture coordinates FileName.
   */
  svtkSetStringMacro(TextureFileName);
  svtkGetStringMacro(TextureFileName);
  //@}

  //@{
  /**
   * Turn on/off the reading of the displacement file.
   */
  svtkSetMacro(ReadDisplacement, svtkTypeBool);
  svtkGetMacro(ReadDisplacement, svtkTypeBool);
  svtkBooleanMacro(ReadDisplacement, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the reading of the scalar file.
   */
  svtkSetMacro(ReadScalar, svtkTypeBool);
  svtkGetMacro(ReadScalar, svtkTypeBool);
  svtkBooleanMacro(ReadScalar, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the reading of the texture coordinate file.
   * Specify name of geometry FileName.
   */
  svtkSetMacro(ReadTexture, svtkTypeBool);
  svtkGetMacro(ReadTexture, svtkTypeBool);
  svtkBooleanMacro(ReadTexture, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the part number to be read.
   */
  svtkSetClampMacro(PartNumber, int, 1, SVTK_INT_MAX);
  svtkGetMacro(PartNumber, int);
  //@}

  /**
   * Returns 1 if this file can be read and 0 if the file cannot be read.
   * Because BYU files do not have anything in the header specifying the file
   * type, the result is not definitive.  Invalid files may still return 1
   * although a valid file will never return 0.
   */
  static int CanReadFile(const char* filename);

protected:
  svtkBYUReader();
  ~svtkBYUReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(svtkDataObject* output, int idx, int numDivisions);

  char* GeometryFileName;
  char* DisplacementFileName;
  char* ScalarFileName;
  char* TextureFileName;
  svtkTypeBool ReadDisplacement;
  svtkTypeBool ReadScalar;
  svtkTypeBool ReadTexture;
  int PartNumber;

  void ReadGeometryFile(FILE* fp, int& numPts, svtkInformation* outInfo);
  void ReadDisplacementFile(int numPts, svtkInformation* outInfo);
  void ReadScalarFile(int numPts, svtkInformation* outInfo);
  void ReadTextureFile(int numPts, svtkInformation* outInfo);

private:
  svtkBYUReader(const svtkBYUReader&) = delete;
  void operator=(const svtkBYUReader&) = delete;
};

#endif
