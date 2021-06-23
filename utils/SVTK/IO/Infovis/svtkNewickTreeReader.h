/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNewickTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkNewickTreeReader
 * @brief   read svtkTree from Newick formatted file
 *
 * svtkNewickTreeReader is a source object that reads Newick tree format
 * files.
 * The output of this reader is a single svtkTree data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @par Thanks:
 * This class is adapted from code originally written by Yu-Wei Wu.
 * @sa
 * svtkTree svtkDataReader
 */

#ifndef svtkNewickTreeReader_h
#define svtkNewickTreeReader_h

#include "svtkDataReader.h"
#include "svtkIOInfovisModule.h" // For export macro

class svtkDoubleArray;
class svtkMutableDirectedGraph;
class svtkStringArray;
class svtkTree;

class SVTKIOINFOVIS_EXPORT svtkNewickTreeReader : public svtkDataReader
{
public:
  static svtkNewickTreeReader* New();
  svtkTypeMacro(svtkNewickTreeReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkTree* GetOutput();
  svtkTree* GetOutput(int idx);
  void SetOutput(svtkTree* output);
  int ReadNewickTree(const char* buffer, svtkTree& tree);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkNewickTreeReader();
  ~svtkNewickTreeReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;
  void CountNodes(const char* buffer, svtkIdType* numNodes);
  svtkIdType BuildTree(char* buffer, svtkMutableDirectedGraph* g, svtkDoubleArray* weights,
    svtkStringArray* names, svtkIdType parent);

private:
  svtkNewickTreeReader(const svtkNewickTreeReader&) = delete;
  void operator=(const svtkNewickTreeReader&) = delete;
};

#endif
