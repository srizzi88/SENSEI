/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetReader
 * @brief   class to read any type of svtk dataset
 *
 * svtkDataSetReader is a class that provides instance variables and methods
 * to read any type of dataset in Visualization Toolkit (svtk) format.  The
 * output type of this class will vary depending upon the type of data
 * file. Convenience methods are provided to keep the data as a particular
 * type. (See text for format description details).
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkDataReader svtkPolyDataReader svtkRectilinearGridReader
 * svtkStructuredPointsReader svtkStructuredGridReader svtkUnstructuredGridReader
 */

#ifndef svtkDataSetReader_h
#define svtkDataSetReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkDataSet;
class svtkPolyData;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkStructuredPoints;
class svtkUnstructuredGrid;

class SVTKIOLEGACY_EXPORT svtkDataSetReader : public svtkDataReader
{
public:
  static svtkDataSetReader* New();
  svtkTypeMacro(svtkDataSetReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this filter
   */
  svtkDataSet* GetOutput();
  svtkDataSet* GetOutput(int idx);
  //@}

  //@{
  /**
   * Get the output as various concrete types. This method is typically used
   * when you know exactly what type of data is being read.  Otherwise, use
   * the general GetOutput() method. If the wrong type is used nullptr is
   * returned.  (You must also set the filename of the object prior to
   * getting the output.)
   */
  svtkPolyData* GetPolyDataOutput();
  svtkStructuredPoints* GetStructuredPointsOutput();
  svtkStructuredGrid* GetStructuredGridOutput();
  svtkUnstructuredGrid* GetUnstructuredGridOutput();
  svtkRectilinearGrid* GetRectilinearGridOutput();
  //@}

  /**
   * This method can be used to find out the type of output expected without
   * needing to read the whole file.
   */
  virtual int ReadOutputType();

  /**
   * Read metadata from file.
   */
  int ReadMetaDataSimple(const std::string& fname, svtkInformation* metadata) override;

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkDataSetReader();
  ~svtkDataSetReader() override;

  svtkDataObject* CreateOutput(svtkDataObject* currentOutput) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkDataSetReader(const svtkDataSetReader&) = delete;
  void operator=(const svtkDataSetReader&) = delete;
};

#endif
