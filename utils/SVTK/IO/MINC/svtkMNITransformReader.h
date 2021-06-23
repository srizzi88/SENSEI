/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMNITransformReader.h

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
 * @class   svtkMNITransformReader
 * @brief   A reader for MNI transformation files.
 *
 * The MNI .xfm file format is used to store geometrical
 * transformations.  Three kinds of transformations are supported by
 * the file format: affine, thin-plate spline, and grid transformations.
 * This file format was developed at the McConnell Brain Imaging Centre
 * at the Montreal Neurological Institute and is used by their software.
 * @sa
 * svtkMINCImageReader svtkMNITransformWriter
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to SVTK.
 */

#ifndef svtkMNITransformReader_h
#define svtkMNITransformReader_h

#include "svtkAlgorithm.h"
#include "svtkIOMINCModule.h" // For export macro

class svtkAbstractTransform;
class svtkDoubleArray;
class svtkCollection;

class SVTKIOMINC_EXPORT svtkMNITransformReader : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkMNITransformReader, svtkAlgorithm);

  static svtkMNITransformReader* New();
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
  virtual const char* GetFileExtensions() { return ".xfm"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI Transform"; }

  /**
   * Test whether the specified file can be read.
   */
  virtual int CanReadFile(const char* name);

  /**
   * Get the number of transforms in the file.
   */
  virtual int GetNumberOfTransforms();

  /**
   * Get one of the transforms listed in the file.
   */
  virtual svtkAbstractTransform* GetNthTransform(int i);

  /**
   * Get the transform that results from concatenating all
   * of the transforms in the file.  This will return null
   * if you have not specified a file name.
   */
  virtual svtkAbstractTransform* GetTransform();

  /**
   * Get any comments that are included in the file.
   */
  virtual const char* GetComments();

protected:
  svtkMNITransformReader();
  ~svtkMNITransformReader() override;

  char* FileName;
  svtkAbstractTransform* Transform;
  svtkCollection* Transforms;
  int LineNumber;
  char* Comments;

  void SetTransform(svtkAbstractTransform* transform);

  int ReadLine(istream& infile, char result[256]);
  int ReadLineAfterComments(istream& infile, char result[256]);
  int SkipWhitespace(istream& infile, char linetext[256], char** cpp);
  int ParseLeftHandSide(istream& infile, char linetext[256], char** cpp, char identifier[256]);
  int ParseStringValue(istream& infile, char linetext[256], char** cpp, char data[256]);
  int ParseFloatValues(istream& infile, char linetext[256], char** cpp, svtkDoubleArray* array);
  int ParseInvertFlagValue(istream& infile, char linetext[256], char** cpp, int* invertFlag);

  int ReadLinearTransform(istream& infile, char linetext[256], char** cp);
  int ReadThinPlateSplineTransform(istream& infile, char linetext[256], char** cp);
  int ReadGridTransform(istream& infile, char linetext[256], char** cp);

  virtual int ReadNextTransform(istream& infile, char linetext[256]);

  virtual int ReadFile();

  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

private:
  svtkMNITransformReader(const svtkMNITransformReader&) = delete;
  void operator=(const svtkMNITransformReader&) = delete;
};

#endif
