/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericDataObjectReader
 * @brief   class to read any type of svtk data object
 *
 * svtkGenericDataObjectReader is a class that provides instance variables and methods
 * to read any type of data object in Visualization Toolkit (svtk) format.  The
 * output type of this class will vary depending upon the type of data
 * file. Convenience methods are provided to return the data as a particular
 * type. (See text for format description details).
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkDataReader svtkGraphReader svtkPolyDataReader svtkRectilinearGridReader
 * svtkStructuredPointsReader svtkStructuredGridReader svtkTableReader
 * svtkTreeReader svtkUnstructuredGridReader
 */

#ifndef svtkGenericDataObjectReader_h
#define svtkGenericDataObjectReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkDataObject;
class svtkGraph;
class svtkMolecule;
class svtkPolyData;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkStructuredPoints;
class svtkTable;
class svtkTree;
class svtkUnstructuredGrid;

class SVTKIOLEGACY_EXPORT svtkGenericDataObjectReader : public svtkDataReader
{
public:
  static svtkGenericDataObjectReader* New();
  svtkTypeMacro(svtkGenericDataObjectReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this filter
   */
  svtkDataObject* GetOutput();
  svtkDataObject* GetOutput(int idx);
  //@}

  //@{
  /**
   * Get the output as various concrete types. This method is typically used
   * when you know exactly what type of data is being read.  Otherwise, use
   * the general GetOutput() method. If the wrong type is used nullptr is
   * returned.  (You must also set the filename of the object prior to
   * getting the output.)
   */
  svtkGraph* GetGraphOutput();
  svtkMolecule* GetMoleculeOutput();
  svtkPolyData* GetPolyDataOutput();
  svtkRectilinearGrid* GetRectilinearGridOutput();
  svtkStructuredGrid* GetStructuredGridOutput();
  svtkStructuredPoints* GetStructuredPointsOutput();
  svtkTable* GetTableOutput();
  svtkTree* GetTreeOutput();
  svtkUnstructuredGrid* GetUnstructuredGridOutput();
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
  svtkGenericDataObjectReader();
  ~svtkGenericDataObjectReader() override;

  svtkDataObject* CreateOutput(svtkDataObject* currentOutput) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkGenericDataObjectReader(const svtkGenericDataObjectReader&) = delete;
  void operator=(const svtkGenericDataObjectReader&) = delete;

  template <typename ReaderT, typename DataT>
  void ReadData(const char* fname, const char* dataClass, svtkDataObject* output);

  svtkSetStringMacro(Header);
};

#endif
