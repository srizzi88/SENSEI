/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSTLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSTLWriter
 * @brief   write stereo lithography files
 *
 * svtkSTLWriter writes stereo lithography (.stl) files in either ASCII or
 * binary form. Stereo lithography files contain only triangles. Since SVTK 8.1,
 * this writer converts non-triangle polygons into triangles, so there is no
 * longer a need to use svtkTriangleFilter prior to using this writer if the
 * input contains polygons with more than three vertices.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * svtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.
 */

#ifndef svtkSTLWriter_h
#define svtkSTLWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkWriter.h"

class svtkCellArray;
class svtkPoints;
class svtkPolyData;
class svtkUnsignedCharArray;

class SVTKIOGEOMETRY_EXPORT svtkSTLWriter : public svtkWriter
{
public:
  static svtkSTLWriter* New();
  svtkTypeMacro(svtkSTLWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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

  //@{
  /**
   * Set the header for the file as text. The header cannot contain 0x00 characters.
   * \sa SetBinaryHeader()
   */
  svtkSetStringMacro(Header);
  svtkGetStringMacro(Header);
  //@}

  //@{
  /**
   * Set binary header for the file.
   * Binary header is only used when writing binary type files.
   * If both Header and BinaryHeader are specified then BinaryHeader is used.
   * Maximum length of binary header is 80 bytes, any content over this limit is ignored.
   */
  virtual void SetBinaryHeader(svtkUnsignedCharArray* binaryHeader);
  svtkGetObjectMacro(BinaryHeader, svtkUnsignedCharArray);
  //@}

  //@{
  /**
   * Specify file type (ASCII or BINARY) for svtk data file.
   */
  svtkSetClampMacro(FileType, int, SVTK_ASCII, SVTK_BINARY);
  svtkGetMacro(FileType, int);
  void SetFileTypeToASCII() { this->SetFileType(SVTK_ASCII); }
  void SetFileTypeToBinary() { this->SetFileType(SVTK_BINARY); }
  //@}

protected:
  svtkSTLWriter();
  ~svtkSTLWriter() override;

  void WriteData() override;

  void WriteBinarySTL(svtkPoints* pts, svtkCellArray* polys, svtkCellArray* strips);
  void WriteAsciiSTL(svtkPoints* pts, svtkCellArray* polys, svtkCellArray* strips);

  char* FileName;
  char* Header;
  svtkUnsignedCharArray* BinaryHeader;
  int FileType;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkSTLWriter(const svtkSTLWriter&) = delete;
  void operator=(const svtkSTLWriter&) = delete;
};

#endif
