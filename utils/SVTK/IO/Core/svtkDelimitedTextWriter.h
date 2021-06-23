/*=========================================================================

  Program:   ParaView
  Module:    svtkDelimitedTextWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkDelimitedTextWriter
 * @brief   Delimited text writer for svtkTable
 * Writes a svtkTable as a delimited text file (such as CSV).
 */

#ifndef svtkDelimitedTextWriter_h
#define svtkDelimitedTextWriter_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkWriter.h"

class svtkStdString;
class svtkTable;

class SVTKIOCORE_EXPORT svtkDelimitedTextWriter : public svtkWriter
{
public:
  static svtkDelimitedTextWriter* New();
  svtkTypeMacro(svtkDelimitedTextWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the delimiter use to separate fields ("," by default.)
   */
  svtkSetStringMacro(FieldDelimiter);
  svtkGetStringMacro(FieldDelimiter);
  //@}

  //@{
  /**
   * Get/Set the delimiter used for string data, if any
   * eg. double quotes(").
   */
  svtkSetStringMacro(StringDelimiter);
  svtkGetStringMacro(StringDelimiter);
  //@}

  //@{
  /**
   * Get/Set the filename for the file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set if StringDelimiter must be used for string data.
   * True by default.
   */
  svtkSetMacro(UseStringDelimiter, bool);
  svtkGetMacro(UseStringDelimiter, bool);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  svtkSetMacro(WriteToOutputString, bool);
  svtkGetMacro(WriteToOutputString, bool);
  svtkBooleanMacro(WriteToOutputString, bool);
  //@}

  /**
   * This convenience method returns the string, sets the IVAR to nullptr,
   * so that the user is responsible for deleting the string.
   */
  char* RegisterAndGetOutputString();

  /**
   * Internal method: Returns the "string" with the "StringDelimiter" if
   * UseStringDelimiter is true.
   */
  svtkStdString GetString(svtkStdString string);

protected:
  svtkDelimitedTextWriter();
  ~svtkDelimitedTextWriter() override;

  bool WriteToOutputString;
  char* OutputString;

  bool OpenStream();

  void WriteData() override;
  virtual void WriteTable(svtkTable* rectilinearGrid);

  // see algorithm for more info.
  // This writer takes in svtkTable.
  int FillInputPortInformation(int port, svtkInformation* info) override;

  char* FileName;
  char* FieldDelimiter;
  char* StringDelimiter;
  bool UseStringDelimiter;

  ostream* Stream;

private:
  svtkDelimitedTextWriter(const svtkDelimitedTextWriter&) = delete;
  void operator=(const svtkDelimitedTextWriter&) = delete;
};

#endif
