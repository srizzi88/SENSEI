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
 * @class   svtkPDALReader
 * @brief   Reads LIDAR data using the PDAL library.
 *
 * svtkPDALReader reads LIDAR data using the PDAL library.  See the
 * readers section on www.pdal.io for the supported formats. It produces a
 * svtkPolyData with point data arrays for attributes such as Intensity,
 * Classification, Color, ...
 *
 *
 * @sa
 * svtkPolyData
 */

#ifndef svtkPDALReader_h
#define svtkPDALReader_h

#include <svtkIOPDALModule.h> // For export macro

#include <svtkPolyDataAlgorithm.h>

namespace pdal
{
class Stage;
};

class SVTKIOPDAL_EXPORT svtkPDALReader : public svtkPolyDataAlgorithm
{
public:
  svtkPDALReader(const svtkPDALReader&) = delete;
  void operator=(const svtkPDALReader&) = delete;
  static svtkPDALReader* New();
  svtkTypeMacro(svtkPDALReader, svtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Name of the file that will be opened
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

protected:
  svtkPDALReader();
  ~svtkPDALReader() override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(pdal::Stage& reader, svtkPolyData* pointsPolyData);

  char* FileName;
};

#endif // svtkPDALReader_h
