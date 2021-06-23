/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSILBuilder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSILBuilder
 * @brief   helper class to build a SIL i.e. a directed graph used
 * by reader producing composite datasets to describes the relationships among
 * the blocks.
 *
 * svtkSILBuilder is a helper class to build a SIL i.e. a directed graph used
 * by reader producing composite datasets to describes the relationships among
 * the blocks.
 * Refer to http://www.paraview.org/Wiki/Block_Hierarchy_Meta_Data for details.
 */

#ifndef svtkSILBuilder_h
#define svtkSILBuilder_h

#include "svtkIOXdmf2Module.h" // For export macro
#include "svtkObject.h"

class svtkUnsignedCharArray;
class svtkStringArray;
class svtkMutableDirectedGraph;

class SVTKIOXDMF2_EXPORT svtkSILBuilder : public svtkObject
{
public:
  static svtkSILBuilder* New();
  svtkTypeMacro(svtkSILBuilder, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the graph to populate.
   */
  void SetSIL(svtkMutableDirectedGraph*);
  svtkGetObjectMacro(SIL, svtkMutableDirectedGraph);
  //@}

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
  svtkGetMacro(RootVertex, svtkIdType);
  //@}

protected:
  svtkSILBuilder();
  ~svtkSILBuilder() override;

  svtkStringArray* NamesArray;
  svtkUnsignedCharArray* CrossEdgesArray;
  svtkMutableDirectedGraph* SIL;

  svtkIdType RootVertex;

private:
  svtkSILBuilder(const svtkSILBuilder&) = delete;
  void operator=(const svtkSILBuilder&) = delete;
};

#endif
