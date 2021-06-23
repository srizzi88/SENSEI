/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimplePointsReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimplePointsReader
 * @brief   Read a list of points from a file.
 *
 * svtkSimplePointsReader is a source object that reads a list of
 * points from a file.  Each point is specified by three
 * floating-point values in ASCII format.  There is one point per line
 * of the file.  A vertex cell is created for each point in the
 * output.  This reader is meant as an example of how to write a
 * reader in SVTK.
 */

#ifndef svtkSimplePointsReader_h
#define svtkSimplePointsReader_h

#include "svtkIOLegacyModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKIOLEGACY_EXPORT svtkSimplePointsReader : public svtkPolyDataAlgorithm
{
public:
  static svtkSimplePointsReader* New();
  svtkTypeMacro(svtkSimplePointsReader, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkSimplePointsReader();
  ~svtkSimplePointsReader() override;

  char* FileName;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSimplePointsReader(const svtkSimplePointsReader&) = delete;
  void operator=(const svtkSimplePointsReader&) = delete;
};

#endif
