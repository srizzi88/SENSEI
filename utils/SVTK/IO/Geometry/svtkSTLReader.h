/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSTLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSTLReader
 * @brief   read ASCII or binary stereo lithography files
 *
 * svtkSTLReader is a source object that reads ASCII or binary stereo
 * lithography files (.stl files). The FileName must be specified to
 * svtkSTLReader. The object automatically detects whether the file is
 * ASCII or binary.
 *
 * .stl files are quite inefficient since they duplicate vertex
 * definitions. By setting the Merging boolean you can control whether the
 * point data is merged after reading. Merging is performed by default,
 * however, merging requires a large amount of temporary storage since a
 * 3D hash table must be constructed.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * svtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.
 */

#ifndef svtkSTLReader_h
#define svtkSTLReader_h

#include "svtkAbstractPolyDataReader.h"
#include "svtkIOGeometryModule.h" // For export macro

class svtkCellArray;
class svtkFloatArray;
class svtkIncrementalPointLocator;
class svtkPoints;

class SVTKIOGEOMETRY_EXPORT svtkSTLReader : public svtkAbstractPolyDataReader
{
public:
  svtkTypeMacro(svtkSTLReader, svtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with merging set to true.
   */
  static svtkSTLReader* New();

  /**
   * Overload standard modified time function. If locator is modified,
   * then this object is modified as well.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Turn on/off merging of points/triangles.
   */
  svtkSetMacro(Merging, svtkTypeBool);
  svtkGetMacro(Merging, svtkTypeBool);
  svtkBooleanMacro(Merging, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off tagging of solids with scalars.
   */
  svtkSetMacro(ScalarTags, svtkTypeBool);
  svtkGetMacro(ScalarTags, svtkTypeBool);
  svtkBooleanMacro(ScalarTags, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points. By
   * default an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Get header string.
   * If an ASCII STL file contains multiple solids then
   * headers are separated by newline character.
   * If a binary STL file is read, the first zero-terminated
   * string is stored in this header, the full header is available
   * by using GetBinaryHeader().
   * \sa GetBinaryHeader()
   */
  svtkGetStringMacro(Header);

  /**
   * Get binary file header string.
   * If ASCII STL file is read then BinaryHeader is not set,
   * and the header can be retrieved using.GetHeader() instead.
   * \sa GetHeader()
   */
  svtkGetObjectMacro(BinaryHeader, svtkUnsignedCharArray);

protected:
  svtkSTLReader();
  ~svtkSTLReader() override;

  /**
   * Create default locator. Used to create one when none is specified.
   */
  svtkIncrementalPointLocator* NewDefaultLocator();

  /**
   * Set header string. Internal use only.
   */
  svtkSetStringMacro(Header);
  virtual void SetBinaryHeader(svtkUnsignedCharArray* binaryHeader);

  svtkTypeBool Merging;
  svtkTypeBool ScalarTags;
  svtkIncrementalPointLocator* Locator;
  char* Header;
  svtkUnsignedCharArray* BinaryHeader;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  bool ReadBinarySTL(FILE* fp, svtkPoints*, svtkCellArray*);
  bool ReadASCIISTL(FILE* fp, svtkPoints*, svtkCellArray*, svtkFloatArray* scalars = nullptr);
  int GetSTLFileType(const char* filename);

private:
  svtkSTLReader(const svtkSTLReader&) = delete;
  void operator=(const svtkSTLReader&) = delete;
};

#endif
