/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJSONDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkJSONDataSetWriter
 * @brief   write svtkDataSet using a svtkArchiver with a JSON meta file along
 *          with all the binary arrays written as standalone binary files.
 *          The generated format can be used by svtk.js using the reader below
 *          https://kitware.github.io/svtk-js/examples/HttpDataSetReader.html
 *
 * svtkJSONDataSetWriter writes svtkImageData / svtkPolyData into a set of files
 * representing each arrays that compose the dataset along with a JSON meta file
 * that describe what they are and how they should be assembled into an actual
 * svtkDataSet.
 *
 *
 * @warning
 * This writer assume LittleEndian by default. Additional work should be done to
 * properly
 * handle endianness.
 *
 *
 * @sa
 * svtkArchiver
 */

#ifndef svtkJSONDataSetWriter_h
#define svtkJSONDataSetWriter_h

#include "svtkIOExportModule.h" // For export macro

#include "svtkWriter.h"

#include <string> // std::string used as parameters in a few methods

class svtkDataSet;
class svtkDataArray;
class svtkDataSetAttributes;
class svtkArchiver;

class SVTKIOEXPORT_EXPORT svtkJSONDataSetWriter : public svtkWriter
{
public:
  using svtkWriter::Write;

  //@{
  /**
   * Compute a MD5 digest of a void/(const unsigned char) pointer to compute a
   *  string hash
   */
  static void ComputeMD5(const unsigned char* content, int size, std::string& hash);
  //@}

  //@{
  /**
   * Compute the target JavaScript typed array name for the given svtkDataArray
   * (Uin8, Uint16, Uin32, Int8, Int16, Int32, Float32, Float64) or
   * "xxx" if no match found
   *
   * Since Uint64 and Int64 does not exist in JavaScript, the needConversion
   * argument will be set to true and Uint32/Int32 will be returned instead.
   */
  static std::string GetShortType(svtkDataArray* input, bool& needConversion);
  //@}

  //@{
  /**
   * Return a Unique identifier for that array
   * (i.e.: Float32_356_13f880891af7b77262c49cae09a41e28 )
   */
  static std::string GetUID(svtkDataArray*, bool& needConversion);
  //@}

  //@{
  /**
   * Return a Unique identifier for any invalid string
   */
  std::string GetValidString(const char*);
  //@}

  //@{
  /**
   * Write the contents of the svtkDataArray to disk based on the filePath
   * provided without any extra information. Just the raw data will be
   * written.
   *
   * If svtkDataArray is a Uint64 or Int64, the data will be converted
   * to Uint32 or Int32 before being written.
   */
  bool WriteArrayContents(svtkDataArray*, const char* relativeFilePath);
  //@}

  //@{
  /**
   * For backwards compatiblity, this static method writes a data array's
   * contents directly to a file.
   */
  static bool WriteArrayAsRAW(svtkDataArray*, const char* filePath);
  //@}

  static svtkJSONDataSetWriter* New();
  svtkTypeMacro(svtkJSONDataSetWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of svtk data file to write.
   * This correspond to the root directory of the data to write.
   * DEPRECATED: use the Archive's API instead.
   */
  SVTK_LEGACY(void SetFileName(const char*));
  SVTK_LEGACY(virtual char* GetFileName());
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  svtkDataSet* GetInput();
  svtkDataSet* GetInput(int port);
  //@}

  //@{
  /**
   * Specify the Scene Archiver object
   */
  virtual void SetArchiver(svtkArchiver*);
  svtkGetObjectMacro(Archiver, svtkArchiver);
  //@}

  void Write(svtkDataSet*);

  bool IsDataSetValid() { return this->ValidDataSet; }

protected:
  svtkJSONDataSetWriter();
  ~svtkJSONDataSetWriter() override;

  void WriteData() final;
  std::string WriteArray(svtkDataArray*, const char* className, const char* arrayName = nullptr);
  std::string WriteDataSetAttributes(svtkDataSetAttributes* fields, const char* className);

  svtkArchiver* Archiver;
  bool ValidDataSet;
  int ValidStringCount;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkJSONDataSetWriter(const svtkJSONDataSetWriter&) = delete;
  void operator=(const svtkJSONDataSetWriter&) = delete;
};

#endif
