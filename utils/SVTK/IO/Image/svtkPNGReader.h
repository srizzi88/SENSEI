/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPNGReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPNGReader
 * @brief   read PNG files
 *
 * svtkPNGReader is a source object that reads PNG files.
 * It should be able to read most any PNG file
 *
 * @sa
 * svtkPNGWriter
 */

#ifndef svtkPNGReader_h
#define svtkPNGReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

class SVTKIOIMAGE_EXPORT svtkPNGReader : public svtkImageReader2
{
public:
  static svtkPNGReader* New();
  svtkTypeMacro(svtkPNGReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Is the given file a PNG file?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".png"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "PNG"; }

  /**
   * Given a 'key' for the text chunks, fills in 'beginEndIndex'
   * with the begin and end indexes. Values are stored between
   * [begin, end) indexes.
   */
  void GetTextChunks(const char* key, int beginEndIndex[2]);
  /**
   * Returns the text key stored at 'index'.
   */
  const char* GetTextKey(int index);
  /**
   * Returns the text value stored at 'index'. A range of indexes
   * that store values for a certain key can be obtained by calling
   * GetTextChunks.
   */
  const char* GetTextValue(int index);
  /**
   * Return the number of text chunks in the PNG file.
   * Note that we don't process compressed or international text entries
   */
  size_t GetNumberOfTextChunks();

  //@{
  /**
   * Set/Get if data spacing should be calculated from the PNG file.
   * Use default spacing if the PNG file don't have valid pixel per meter parameters.
   * Default is false.
   */
  svtkSetMacro(ReadSpacingFromFile, bool);
  svtkGetMacro(ReadSpacingFromFile, bool);
  svtkBooleanMacro(ReadSpacingFromFile, bool);
  //@}
protected:
  svtkPNGReader();
  ~svtkPNGReader() override;

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;
  template <class OT>
  void svtkPNGReaderUpdate(svtkImageData* data, OT* outPtr);
  template <class OT>
  void svtkPNGReaderUpdate2(OT* outPtr, int* outExt, svtkIdType* outInc, long pixSize);

private:
  svtkPNGReader(const svtkPNGReader&) = delete;
  void operator=(const svtkPNGReader&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
  bool ReadSpacingFromFile;
};
#endif
