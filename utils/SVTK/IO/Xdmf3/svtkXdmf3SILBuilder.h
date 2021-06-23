/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3SILBuilder.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmf3SILBuilder
 * @brief   helper to allow block selection
 *
 * svtkXdmf3Reader uses this to build up a datastructure that represents
 * block trees that correspond to the file. ParaView builds a GUI from
 * that to let the user select from the various block and types of blocks
 * that should or should not be loaded.
 *
 * This file is a helper for the svtkXdmf3Reader and svtkXdmf3Writer and
 * not intended to be part of SVTK public API
 */

#ifndef svtkXdmf3SILBuilder_h
#define svtkXdmf3SILBuilder_h

#include "svtkIOXdmf3Module.h" // For export macro
#include "svtkType.h"

class svtkMutableDirectedGraph;
class svtkStringArray;
class svtkUnsignedCharArray;

class SVTKIOXDMF3_EXPORT svtkXdmf3SILBuilder
{
public:
  svtkStringArray* NamesArray;
  svtkUnsignedCharArray* CrossEdgesArray;
  svtkMutableDirectedGraph* SIL;
  svtkIdType RootVertex;
  svtkIdType BlocksRoot;
  svtkIdType HierarchyRoot;
  svtkIdType VertexCount;

  /**
   * Initializes the data-structures.
   */
  void Initialize();

  //@{
  /**
   * Add vertex, child-edge or cross-edge to the graph.
   */
  svtkIdType AddVertex(const char* name);
  svtkIdType AddChildEdge(svtkIdType parent, svtkIdType child);
  svtkIdType AddCrossEdge(svtkIdType src, svtkIdType dst);
  //@}

  //@{
  /**
   * Returns the vertex id for the root vertex.
   */
  svtkIdType GetRootVertex();
  svtkIdType GetBlocksRoot();
  svtkIdType GetHierarchyRoot();
  //@}

  bool IsMaxedOut();

  svtkXdmf3SILBuilder();
  ~svtkXdmf3SILBuilder();
  svtkXdmf3SILBuilder(const svtkXdmf3SILBuilder&) = delete;
};

#endif // svtkXdmf3SILBuilder_h
// SVTK-HeaderTest-Exclude: svtkXdmf3SILBuilder.h
