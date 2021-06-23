/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChacoGraphReader.h

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
 * @class   svtkChacoGraphReader
 * @brief   Reads chaco graph files.
 *
 *
 * svtkChacoGraphReader reads in files in the Chaco format into a svtkGraph.
 * An example is the following
 * <code>
 * 10 13
 * 2 6 10
 * 1 3
 * 2 4 8
 * 3 5
 * 4 6 10
 * 1 5 7
 * 6 8
 * 3 7 9
 * 8 10
 * 1 5 9
 * </code>
 * The first line specifies the number of vertices and edges
 * in the graph. Each additional line contains the vertices adjacent
 * to a particular vertex.  In this example, vertex 1 is adjacent to
 * 2, 6 and 10, vertex 2 is adjacent to 1 and 3, etc.  Since Chaco ids
 * start at 1 and SVTK ids start at 0, the vertex ids in the svtkGraph
 * will be 1 less than the Chaco ids.
 */

#ifndef svtkChacoGraphReader_h
#define svtkChacoGraphReader_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkUndirectedGraphAlgorithm.h"

class SVTKIOINFOVIS_EXPORT svtkChacoGraphReader : public svtkUndirectedGraphAlgorithm
{
public:
  static svtkChacoGraphReader* New();
  svtkTypeMacro(svtkChacoGraphReader, svtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The Chaco file name.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

protected:
  svtkChacoGraphReader();
  ~svtkChacoGraphReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  char* FileName;

  svtkChacoGraphReader(const svtkChacoGraphReader&) = delete;
  void operator=(const svtkChacoGraphReader&) = delete;
};

#endif // svtkChacoGraphReader_h
