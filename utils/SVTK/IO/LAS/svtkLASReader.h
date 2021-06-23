/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGDALRasterReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLASRasterReader
 * @brief   Reads LIDAR data saved using the LAS file format.
 *
 * svtkLASReader is a source object that reads LIDAR data saved using
 * the LAS file format. This reader uses the libLAS library.
 * It produces a svtkPolyData with point data arrays:
 * "intensity": svtkUnsignedShortArray
 * "classification": svtkUnsignedCharArray (optional)
 * "color": svtkUnsignedShortArray (optional)
 *
 *
 * @sa
 * svtkPolyData
 */

#ifndef svtkLASReader_h
#define svtkLASReader_h

#include <svtkIOLASModule.h> // For export macro

#include <svtkPolyDataAlgorithm.h>

namespace liblas
{
class Header;
class Reader;
};

class SVTKIOLAS_EXPORT svtkLASReader : public svtkPolyDataAlgorithm
{
public:
  svtkLASReader(const svtkLASReader&) = delete;
  void operator=(const svtkLASReader&) = delete;
  static svtkLASReader* New();
  svtkTypeMacro(svtkLASReader, svtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Accessor for name of the file that will be opened
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

protected:
  svtkLASReader();
  ~svtkLASReader() override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(liblas::Reader& reader, svtkPolyData* pointsPolyData);

  char* FileName;
};

#endif // svtkLASReader_h
