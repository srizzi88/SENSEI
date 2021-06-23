/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIMACSGraphReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkDIMACSGraphReader
 * @brief   reads svtkGraph data from a DIMACS
 * formatted file
 *
 *
 * svtkDIMACSGraphReader is a source object that reads svtkGraph data files
 * from a DIMACS format.
 *
 * The reader has special handlers for max-flow and graph coloring problems,
 * which are specified in the problem line as 'max' and 'edge' respectively.
 * Other graphs are treated as generic DIMACS files.
 *
 * DIMACS formatted files consist of lines in which the first character in
 * in column 0 specifies the type of the line.
 *
 * Generic DIMACS files have the following line types:
 * - problem statement line : p graph num_verts num_edges
 * - node line (optional)   : n node_id node_weight
 * - edge line              : a src_id trg_id edge_weight
 * - alternate edge format  : e src_id trg_id edge_weight
 * - comment lines          : c I am a comment line
 * ** note, there should be one and only one problem statement line per file.
 *
 *
 * DIMACS graphs are undirected and nodes are numbered 1..n
 *
 * See webpage for additional formatting details.
 * -  http://dimacs.rutgers.edu/Challenges/
 * -  http://www.dis.uniroma1.it/~challenge9/format.shtml
 *
 * @sa
 * svtkDIMACSGraphWriter
 *
 */

#ifndef svtkDIMACSGraphReader_h
#define svtkDIMACSGraphReader_h

#include "svtkGraphAlgorithm.h"
#include "svtkIOInfovisModule.h" // For export macro
#include "svtkStdString.h"       // For string API

class SVTKIOINFOVIS_EXPORT svtkDIMACSGraphReader : public svtkGraphAlgorithm
{

public:
  static svtkDIMACSGraphReader* New();
  svtkTypeMacro(svtkDIMACSGraphReader, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The DIMACS file name.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * Vertex attribute array name
   */
  svtkGetStringMacro(VertexAttributeArrayName);
  svtkSetStringMacro(VertexAttributeArrayName);
  //@}

  //@{
  /**
   * Edge attribute array name
   */
  svtkGetStringMacro(EdgeAttributeArrayName);
  svtkSetStringMacro(EdgeAttributeArrayName);
  //@}

protected:
  svtkDIMACSGraphReader();
  ~svtkDIMACSGraphReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int buildGenericGraph(svtkGraph* output, svtkStdString& defaultVertexAttrArrayName,
    svtkStdString& defaultEdgeAttrArrayName);

  int buildColoringGraph(svtkGraph* output);
  int buildMaxflowGraph(svtkGraph* output);

  /**
   * Creates directed or undirected output based on Directed flag.
   */
  int RequestDataObject(svtkInformation*, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int ReadGraphMetaData();

private:
  bool fileOk;
  bool Directed;
  char* FileName;
  char* VertexAttributeArrayName;
  char* EdgeAttributeArrayName;

  int numVerts;
  int numEdges;
  svtkStdString dimacsProblemStr;

  svtkDIMACSGraphReader(const svtkDIMACSGraphReader&) = delete;
  void operator=(const svtkDIMACSGraphReader&) = delete;
};

#endif // svtkDIMACSGraphReader_h
