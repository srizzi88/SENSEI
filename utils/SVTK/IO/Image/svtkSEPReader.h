/*=========================================================================
  Copyright (c) GeometryFactory
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkSEPReader
 * @brief Stanford Exploration Project files reader.
 *
 * This reader takes a .H file that points to a .H@ file and contains
 * all the information to interpret the raw data in the  .H@ file.
 * The only supported data_format are xdr_float and native_float,
 * with a esize of 4.
 */

#ifndef svtkSEPReader_h
#define svtkSEPReader_h

#include <svtkImageReader.h>

#include <string> //for string

class SVTKIOIMAGE_EXPORT svtkSEPReader : public svtkImageReader
{
public:
  static svtkSEPReader* New();
  svtkTypeMacro(svtkSEPReader, svtkImageReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Check if the given file is a .H file
   */
  int CanReadFile(const char* fname) override;

  const char* GetFileExtensions() override { return ".H"; }

protected:
  svtkSEPReader();
  ~svtkSEPReader() override = default;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int ReadHeader();

  std::string DataFile;

private:
  svtkSEPReader(const svtkSEPReader&) = delete;
  void operator=(const svtkSEPReader&) = delete;
};

#endif
