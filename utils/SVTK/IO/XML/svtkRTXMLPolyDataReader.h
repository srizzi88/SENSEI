/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRTXMLPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRTXMLPolyDataReader
 * @brief   Read RealTime SVTK XML PolyData files.
 *
 * svtkRTXMLPolyDataReader reads the SVTK XML PolyData file format in real time.
 *
 */

#ifndef svtkRTXMLPolyDataReader_h
#define svtkRTXMLPolyDataReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLPolyDataReader.h"

class svtkRTXMLPolyDataReaderInternals;

class SVTKIOXML_EXPORT svtkRTXMLPolyDataReader : public svtkXMLPolyDataReader
{
public:
  svtkTypeMacro(svtkRTXMLPolyDataReader, svtkXMLPolyDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkRTXMLPolyDataReader* New();

  // This sets the DataLocation and also
  // Reset the reader by calling ResetReader()
  void SetLocation(const char* dataLocation);
  svtkGetStringMacro(DataLocation);

  /**
   * Reader will read in the next available data file
   * The filename is this->NextFileName maintained internally
   */
  virtual void UpdateToNextFile();

  /**
   * check if there is new data file available in the
   * given DataLocation
   */
  virtual int NewDataAvailable();

  /**
   * ResetReader check the data directory specified in
   * this->DataLocation, and reset the Internal data structure
   * specifically: this->Internal->ProcessedFileList
   * for monitoring the arriving new data files
   * if SetDataLocation(char*) is set by the user,
   * this ResetReader() should also be invoked.
   */
  virtual void ResetReader();

  /**
   * Return the name of the next available data file
   * assume NewDataAvailable() return SVTK_OK
   */
  const char* GetNextFileName();

protected:
  svtkRTXMLPolyDataReader();
  ~svtkRTXMLPolyDataReader() override;

  //@{
  /**
   * Get/Set the location of the input data files.
   */
  svtkSetStringMacro(DataLocation);
  //@}

  void InitializeToCurrentDir();
  int IsProcessed(const char*);
  char* GetDataFileFullPathName(const char*);

  //@{
  /**
   * the DataLocation should be set and ResetReader()
   * should be called after SetDataLocation
   */
  char* DataLocation;
  svtkRTXMLPolyDataReaderInternals* Internal;
  //@}

private:
  svtkRTXMLPolyDataReader(const svtkRTXMLPolyDataReader&) = delete;
  void operator=(const svtkRTXMLPolyDataReader&) = delete;
};

#endif
