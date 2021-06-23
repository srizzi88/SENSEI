/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiNewickTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiNewickTreeReader
 * @brief   read multiple svtkTrees from Newick formatted file
 *
 * svtkMultiNewickTreeReader is a source object that reads Newick tree format
 * files.
 * The output of this reader is a single svtkMultiPieceDataSet that contains multiple svtkTree
 * objects. The superclass of this class, svtkDataReader, provides many methods for controlling the
 * reading of the data file, see svtkDataReader for more information.
 * @sa
 * svtkTree svtkDataReader
 */

#ifndef svtkMultiNewickTreeReader_h
#define svtkMultiNewickTreeReader_h

#include "svtkDataReader.h"
#include "svtkIOInfovisModule.h" // For export macro

class svtkMultiPieceDataSet;
class svtkNewickTreeReader;
class SVTKIOINFOVIS_EXPORT svtkMultiNewickTreeReader : public svtkDataReader
{
public:
  static svtkMultiNewickTreeReader* New();
  svtkTypeMacro(svtkMultiNewickTreeReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkMultiPieceDataSet* GetOutput();
  svtkMultiPieceDataSet* GetOutput(int idx);
  void SetOutput(svtkMultiPieceDataSet* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkMultiNewickTreeReader();
  ~svtkMultiNewickTreeReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkMultiNewickTreeReader(const svtkMultiNewickTreeReader&) = delete;
  void operator=(const svtkMultiNewickTreeReader&) = delete;
};

#endif
