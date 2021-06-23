/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMNIObjectReader.h

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
 * @class   svtkMNIObjectReader
 * @brief   A reader for MNI surface mesh files.
 *
 * The MNI .obj file format is used to store geometrical data.  This
 * file format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Only polygon and line files are supported by this reader, but for
 * those formats, all data elements are read including normals, colors,
 * and surface properties.  ASCII and binary file types are supported.
 * @sa
 * svtkMINCImageReader svtkMNIObjectWriter svtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to SVTK.
 */

#ifndef svtkMNIObjectReader_h
#define svtkMNIObjectReader_h

#include "svtkIOMINCModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkProperty;
class svtkPolyData;
class svtkFloatArray;
class svtkIntArray;
class svtkPoints;
class svtkCellArray;

class SVTKIOMINC_EXPORT svtkMNIObjectReader : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkMNIObjectReader, svtkPolyDataAlgorithm);

  static svtkMNIObjectReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the file name.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".obj"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI object"; }

  /**
   * Test whether the specified file can be read.
   */
  virtual int CanReadFile(const char* name);

  /**
   * Get the property associated with the object.
   */
  virtual svtkProperty* GetProperty() { return this->Property; }

protected:
  svtkMNIObjectReader();
  ~svtkMNIObjectReader() override;

  char* FileName;
  svtkProperty* Property;
  int FileType;

  istream* InputStream;
  int LineNumber;
  char* LineText;
  char* CharPointer;

  int ReadLine(char* text, unsigned int length);
  int SkipWhitespace();
  int ParseValues(svtkDataArray* array, svtkIdType n);
  int ParseIdValue(svtkIdType* value);

  int ReadNumberOfPoints(svtkIdType* numCells);
  int ReadNumberOfCells(svtkIdType* numCells);
  int ReadProperty(svtkProperty* property);
  int ReadLineThickness(svtkProperty* property);
  int ReadPoints(svtkPolyData* polyData, svtkIdType numPoints);
  int ReadNormals(svtkPolyData* polyData, svtkIdType numPoints);
  int ReadColors(svtkProperty* property, svtkPolyData* data, svtkIdType numPoints, svtkIdType numCells);
  int ReadCells(svtkPolyData* data, svtkIdType numCells, int cellType);

  int ReadPolygonObject(svtkPolyData* output);
  int ReadLineObject(svtkPolyData* output);

  virtual int ReadFile(svtkPolyData* output);

  int RequestData(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

private:
  svtkMNIObjectReader(const svtkMNIObjectReader&) = delete;
  void operator=(const svtkMNIObjectReader&) = delete;
};

#endif
