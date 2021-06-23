/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayDataReader.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkArrayDataReader
 * @brief    Reads svtkArrayData written by svtkArrayDataWriter.
 *
 *
 * Reads svtkArrayData data written with svtkArrayDataWriter.
 *
 * Outputs:
 *   Output port 0: svtkArrayData containing a collection of svtkArrays.
 *
 * @sa
 * svtkArrayDataWriter
 */

#ifndef svtkArrayDataReader_h
#define svtkArrayDataReader_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkIOCoreModule.h" // For export macro

class SVTKIOCORE_EXPORT svtkArrayDataReader : public svtkArrayDataAlgorithm
{
public:
  static svtkArrayDataReader* New();
  svtkTypeMacro(svtkArrayDataReader, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the filesystem location from which data will be read.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * The input string to parse. If you set the input string, you must also set
   * the ReadFromInputString flag to parse the string instead of a file.
   */
  virtual void SetInputString(const svtkStdString& string);
  virtual svtkStdString GetInputString();
  //@}

  //@{
  /**
   * Whether to read from an input string as opposed to a file, which is the default.
   */
  svtkSetMacro(ReadFromInputString, bool);
  svtkGetMacro(ReadFromInputString, bool);
  svtkBooleanMacro(ReadFromInputString, bool);
  //@}

  /**
   * Read an arbitrary array from a stream.  Note: you MUST always
   * open streams in binary mode to prevent problems reading files
   * on Windows.
   */
  static svtkArrayData* Read(istream& stream);

  /**
   * Read an arbitrary array from a string.
   */
  static svtkArrayData* Read(const svtkStdString& str);

protected:
  svtkArrayDataReader();
  ~svtkArrayDataReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  svtkStdString InputString;
  bool ReadFromInputString;

private:
  svtkArrayDataReader(const svtkArrayDataReader&) = delete;
  void operator=(const svtkArrayDataReader&) = delete;
};

#endif
