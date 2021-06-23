/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoJSONWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGeoJSONWriter
 * @brief   Convert svtkPolyData to Geo JSON format.
 *
 * Outputs a Geo JSON (http://www.geojson.org) description of the input
 * polydata data set.
 */

#ifndef svtkGeoJSONWriter_h
#define svtkGeoJSONWriter_h

#include "svtkIOGeoJSONModule.h" // For export macro
#include "svtkWriter.h"

class svtkLookupTable;

class SVTKIOGEOJSON_EXPORT svtkGeoJSONWriter : public svtkWriter
{
public:
  static svtkGeoJSONWriter* New();
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkGeoJSONWriter, svtkWriter);

  //@{
  /**
   * Accessor for name of the file that will be opened on WriteData
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  svtkSetMacro(WriteToOutputString, bool);
  svtkGetMacro(WriteToOutputString, bool);
  svtkBooleanMacro(WriteToOutputString, bool);
  //@}

  //@{
  /**
   * When WriteToOutputString in on, then a string is allocated, written to,
   * and can be retrieved with these methods.  The string is deleted during
   * the next call to write ...
   */
  svtkGetMacro(OutputStringLength, int);
  svtkGetStringMacro(OutputString);
  unsigned char* GetBinaryOutputString()
  {
    return reinterpret_cast<unsigned char*>(this->OutputString);
  }
  //@}

  //@{
  /**
   * Controls how data attributes are written out.
   * When 0, data attributes are ignored and not written at all.
   * When 1, values are mapped through a lookup table and colors are written to the output.
   * When 2, which is the default, the values are written directly.
   */
  svtkSetMacro(ScalarFormat, int);
  svtkGetMacro(ScalarFormat, int);
  //@}

  //@{
  /**
   * Controls the lookup table to use when ValueMode is set to map colors;
   */
  void SetLookupTable(svtkLookupTable* lut);
  svtkGetObjectMacro(LookupTable, svtkLookupTable);
  //@}

  /**
   * When WriteToOutputString is on, this method returns a copy of the
   * output string in a svtkStdString.
   */
  svtkStdString GetOutputStdString();

  /**
   * This convenience method returns the string, sets the IVAR to nullptr,
   * so that the user is responsible for deleting the string.
   * I am not sure what the name should be, so it may change in the future.
   */
  char* RegisterAndGetOutputString();

protected:
  svtkGeoJSONWriter();
  ~svtkGeoJSONWriter() override;

  // Only accepts svtkPolyData
  virtual int FillInputPortInformation(int port, svtkInformation* info) override;

  // Implementation of Write()
  void WriteData() override;

  // Helper for Write that writes attributes out
  void WriteScalar(svtkDataArray* da, svtkIdType ptId);
  svtkLookupTable* LookupTable;

  bool WriteToOutputString;
  char* OutputString;
  int OutputStringLength;

  int ScalarFormat;

  // Internal helpers
  ostream* OpenFile();
  void ConditionalComma(svtkIdType, svtkIdType);
  void CloseFile(ostream*);
  class Internals;
  Internals* WriterHelper;
  char* FileName;

private:
  svtkGeoJSONWriter(const svtkGeoJSONWriter&) = delete;
  void operator=(const svtkGeoJSONWriter&) = delete;
};

#endif // svtkGeoJSONWriter_h
