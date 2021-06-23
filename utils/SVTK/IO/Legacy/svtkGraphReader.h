/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphReader
 * @brief   read svtkGraph data file
 *
 * svtkGraphReader is a source object that reads ASCII or binary
 * svtkGraph data files in svtk format. (see text for format details).
 * The output of this reader is a single svtkGraph data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkGraph svtkDataReader svtkGraphWriter
 */

#ifndef svtkGraphReader_h
#define svtkGraphReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkGraph;

class SVTKIOLEGACY_EXPORT svtkGraphReader : public svtkDataReader
{
public:
  static svtkGraphReader* New();
  svtkTypeMacro(svtkGraphReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkGraph* GetOutput();
  svtkGraph* GetOutput(int idx);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkGraphReader();
  ~svtkGraphReader() override;

  enum GraphType
  {
    UnknownGraph,
    DirectedGraph,
    UndirectedGraph,
    Molecule
  };

  svtkDataObject* CreateOutput(svtkDataObject* currentOutput) override;

  // Read beginning of file to determine whether the graph is directed.
  virtual int ReadGraphType(const char* fname, GraphType& type);

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkGraphReader(const svtkGraphReader&) = delete;
  void operator=(const svtkGraphReader&) = delete;
};

#endif
