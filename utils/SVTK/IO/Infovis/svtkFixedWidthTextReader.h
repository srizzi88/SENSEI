/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedWidthTextReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkFixedWidthTextReader
 * @brief   reader for pulling in text files with fixed-width fields
 *
 *
 *
 * svtkFixedWidthTextReader reads in a table from a text file where
 * each column occupies a certain number of characters.
 *
 * This class emits ProgressEvent for every 100 lines it reads.
 *
 *
 * @warning
 * This first version of the reader will assume that all fields have
 * the same width.  It also assumes that the first line in the file
 * has at least as many fields (i.e. at least as many characters) as
 * any other line in the file.
 *
 * @par Thanks:
 * Thanks to Andy Wilson from Sandia National Laboratories for
 * implementing this class.
 */

#ifndef svtkFixedWidthTextReader_h
#define svtkFixedWidthTextReader_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class svtkCommand;
class svtkTable;

class SVTKIOINFOVIS_EXPORT svtkFixedWidthTextReader : public svtkTableAlgorithm
{
public:
  static svtkFixedWidthTextReader* New();
  svtkTypeMacro(svtkFixedWidthTextReader, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);

  //@{
  /**
   * Set/get the field width
   */
  svtkSetMacro(FieldWidth, int);
  svtkGetMacro(FieldWidth, int);
  //@}

  //@{
  /**
   * If set, this flag will cause the reader to strip whitespace from
   * the beginning and ending of each field.  Defaults to off.
   */
  svtkSetMacro(StripWhiteSpace, bool);
  svtkGetMacro(StripWhiteSpace, bool);
  svtkBooleanMacro(StripWhiteSpace, bool);
  //@}

  //@{
  /**
   * Set/get whether to treat the first line of the file as headers.
   */
  svtkGetMacro(HaveHeaders, bool);
  svtkSetMacro(HaveHeaders, bool);
  svtkBooleanMacro(HaveHeaders, bool);
  //@}

  //@{
  /**
   * Set/get the ErrorObserver for the internal svtkTable
   * This is useful for applications that want to catch error messages.
   */
  void SetTableErrorObserver(svtkCommand*);
  svtkGetObjectMacro(TableErrorObserver, svtkCommand);
  //@}

protected:
  svtkFixedWidthTextReader();
  ~svtkFixedWidthTextReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void OpenFile();

  char* FileName;
  bool HaveHeaders;
  bool StripWhiteSpace;
  int FieldWidth;

private:
  svtkFixedWidthTextReader(const svtkFixedWidthTextReader&) = delete;
  void operator=(const svtkFixedWidthTextReader&) = delete;
  svtkCommand* TableErrorObserver;
};

#endif
