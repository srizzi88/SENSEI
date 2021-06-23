/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVeraOutReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkVeraOutReader - File reader for VERA OUT HDF5 format.

#ifndef svtkVeraOutReader_h
#define svtkVeraOutReader_h

#include "svtkIOVeraOutModule.h" // For SVTKIOVERAOUT_EXPORT macro
#include <vector>               // For STL vector

// svtkCommonExecutionModel
#include "svtkRectilinearGridAlgorithm.h"

class svtkDataArraySelection;

class SVTKIOVERAOUT_EXPORT svtkVeraOutReader : public svtkRectilinearGridAlgorithm
{
public:
  static svtkVeraOutReader* New();
  svtkTypeMacro(svtkVeraOutReader, svtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

  /**
   * Get svtkDataArraySelection instance to select cell arrays to read.
   */
  svtkDataArraySelection* GetCellDataArraySelection() const;
  /**
   * Get svtkDataArraySelection instance to select field arrays to read.
   */
  svtkDataArraySelection* GetFieldDataArraySelection() const;

  /**
   * Override GetMTime because of array selector.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkVeraOutReader();
  ~svtkVeraOutReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // Trigger the real data access
  int RequestData(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector) override;

  char* FileName;
  int NumberOfTimeSteps;
  std::vector<double> TimeSteps;

private:
  svtkVeraOutReader(const svtkVeraOutReader&) = delete;
  void operator=(const svtkVeraOutReader&) = delete;

  class Internals;
  Internals* Internal;
};

#endif
