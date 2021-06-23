/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTulipReader.h

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
 * @class   svtkTulipReader
 * @brief   Reads tulip graph files.
 *
 *
 * svtkTulipReader reads in files in the Tulip format.
 * Definition of the Tulip file format can be found online at:
 * http://tulip.labri.fr/tlpformat.php
 * An example is the following
 * <code>
 * (nodes 0 1 2 3 4 5 6 7 8 9)
 * (edge 0 0 1)
 * (edge 1 1 2)
 * (edge 2 2 3)
 * (edge 3 3 4)
 * (edge 4 4 5)
 * (edge 5 5 6)
 * (edge 6 6 7)
 * (edge 7 7 8)
 * (edge 8 8 9)
 * (edge 9 9 0)
 * (edge 10 0 5)
 * (edge 11 2 7)
 * (edge 12 4 9)
 * </code>
 * where "nodes" defines all the nodes ids in the graph, and "edge"
 * is a triple of edge id, source vertex id, and target vertex id.
 * The graph is read in as undirected graph. Pedigree ids are set on the output
 * graph's vertices and edges that match the node and edge ids defined in the
 * Tulip file.
 *
 * Clusters are output as a svtkAnnotationLayers on output port 1. Each cluster
 * name is used to create an annotation layer, and each cluster with that name
 * is added to the layer as a svtkSelectionNode. Nesting hierarchies are treated
 * as if they were flat. See svtkGraphAnnotationLayersFilter for an example of
 * how the clusters can be represented visually.
 *
 * @attention
 * Only string, int, and double properties are supported. Display information
 * is discarded.
 *
 * @par Thanks:
 * Thanks to Colin Myers, University of Leeds for extending this implementation.
 */

#ifndef svtkTulipReader_h
#define svtkTulipReader_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkUndirectedGraphAlgorithm.h"

class SVTKIOINFOVIS_EXPORT svtkTulipReader : public svtkUndirectedGraphAlgorithm
{
public:
  static svtkTulipReader* New();
  svtkTypeMacro(svtkTulipReader, svtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The Tulip file name.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

protected:
  svtkTulipReader();
  ~svtkTulipReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the outputs to svtkUndirectedGraph and svtkAnnotationLayers.
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  char* FileName;

  svtkTulipReader(const svtkTulipReader&) = delete;
  void operator=(const svtkTulipReader&) = delete;
};

#endif // svtkTulipReader_h
