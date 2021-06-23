/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMNIObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
/**
 * @class   svtkMNIObjectWriter
 * @brief   A writer for MNI surface mesh files.
 *
 * The MNI .obj file format is used to store geometrical data.  This
 * file format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Only polygon and line files are supported by this writer.  For these
 * formats, all data elements are written including normals, colors,
 * and surface properties.  ASCII and binary file types are supported.
 * @sa
 * svtkMINCImageReader svtkMNIObjectReader svtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to SVTK.
 */

#ifndef svtkMNIObjectWriter_h
#define svtkMNIObjectWriter_h

#include "svtkIOMINCModule.h" // For export macro
#include "svtkWriter.h"

class svtkMapper;
class svtkProperty;
class svtkLookupTable;
class svtkPolyData;
class svtkFloatArray;
class svtkIntArray;
class svtkPoints;

class SVTKIOMINC_EXPORT svtkMNIObjectWriter : public svtkWriter
{
public:
  svtkTypeMacro(svtkMNIObjectWriter, svtkWriter);

  static svtkMNIObjectWriter* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".obj"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI object"; }

  //@{
  /**
   * Set the property associated with the object.  Optional.
   * This is useful for exporting an actor.
   */
  virtual void SetProperty(svtkProperty* property);
  virtual svtkProperty* GetProperty() { return this->Property; }
  //@}

  //@{
  /**
   * Set the mapper associated with the object.  Optional.
   * This is useful for exporting an actor with the same colors
   * that are used to display the actor within SVTK.
   */
  virtual void SetMapper(svtkMapper* mapper);
  virtual svtkMapper* GetMapper() { return this->Mapper; }
  //@}

  //@{
  /**
   * Set the lookup table associated with the object.  This will be
   * used to convert scalar values to colors, if a mapper is not set.
   */
  virtual void SetLookupTable(svtkLookupTable* table);
  virtual svtkLookupTable* GetLookupTable() { return this->LookupTable; }
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
  svtkMNIObjectWriter();
  ~svtkMNIObjectWriter() override;

  svtkProperty* Property;
  svtkMapper* Mapper;
  svtkLookupTable* LookupTable;

  ostream* OutputStream;

  int WriteObjectType(int objType);
  int WriteValues(svtkDataArray* array);
  int WriteIdValue(svtkIdType value);
  int WriteNewline();

  int WriteProperty(svtkProperty* property);
  int WriteLineThickness(svtkProperty* property);
  int WritePoints(svtkPolyData* polyData);
  int WriteNormals(svtkPolyData* polyData);
  int WriteColors(svtkProperty* property, svtkMapper* mapper, svtkPolyData* data);
  int WriteCells(svtkPolyData* data, int cellType);

  int WritePolygonObject(svtkPolyData* output);
  int WriteLineObject(svtkPolyData* output);

  void WriteData() override;

  char* FileName;

  int FileType;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  ostream* OpenFile();
  void CloseFile(ostream* fp);

private:
  svtkMNIObjectWriter(const svtkMNIObjectWriter&) = delete;
  void operator=(const svtkMNIObjectWriter&) = delete;
};

#endif
