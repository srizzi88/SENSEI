/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostBreadthFirstSearch.h

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
 * @class   svtkBoostBreadthFirstSearch
 * @brief   Boost breadth_first_search on a svtkGraph
 *
 *
 *
 * This svtk class uses the Boost breadth_first_search
 * generic algorithm to perform a breadth first search from a given
 * a 'source' vertex on the input graph (a svtkGraph).
 *
 * @sa
 * svtkGraph svtkBoostGraphAdapter
 */

#ifndef svtkBoostBreadthFirstSearch_h
#define svtkBoostBreadthFirstSearch_h

#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "svtkStdString.h"                         // For string type
#include "svtkVariant.h"                           // For variant type

#include "svtkGraphAlgorithm.h"

class svtkSelection;

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostBreadthFirstSearch : public svtkGraphAlgorithm
{
public:
  static svtkBoostBreadthFirstSearch* New();
  svtkTypeMacro(svtkBoostBreadthFirstSearch, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Convenience methods for setting the origin selection input.
   */
  void SetOriginSelection(svtkSelection* s);
  void SetOriginSelectionConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }
  //@}

  /**
   * Set the index (into the vertex array) of the
   * breadth first search 'origin' vertex.
   */
  void SetOriginVertex(svtkIdType index);

  /**
   * Set the breadth first search 'origin' vertex.
   * This method is basically the same as above
   * but allows the application to simply specify
   * an array name and value, instead of having to
   * know the specific index of the vertex.
   */
  void SetOriginVertex(svtkStdString arrayName, svtkVariant value);

  /**
   * Convenience method for setting the origin vertex
   * given an array name and string value.
   * This method is primarily for the benefit of the
   * SVTK Parallel client/server layer, callers should
   * prefer to use SetOriginVertex() whenever possible.
   */
  void SetOriginVertexString(char* arrayName, char* value);

  //@{
  /**
   * Set the output array name. If no output array name is
   * set then the name 'BFS' is used.
   */
  svtkSetStringMacro(OutputArrayName);
  //@}

  //@{
  /**
   * Use the svtkSelection from input port 1 as the origin vertex.
   * The selection should be a IDS selection with field type POINTS.
   * The first ID in the selection will be used for the origin vertex.
   * Default is off (origin is specified by SetOriginVertex(...)).
   */
  svtkSetMacro(OriginFromSelection, bool);
  svtkGetMacro(OriginFromSelection, bool);
  svtkBooleanMacro(OriginFromSelection, bool);
  //@}

  //@{
  /**
   * Create an output selection containing the ID of a vertex based
   * on the output selection type. The default is to use the
   * the maximum distance from the starting vertex.  Defaults to off.
   */
  svtkGetMacro(OutputSelection, bool);
  svtkSetMacro(OutputSelection, bool);
  svtkBooleanMacro(OutputSelection, bool);
  //@}

  //@{
  /**
   * Set the output selection type. The default is to use the
   * the maximum distance from the starting vertex "MAX_DIST_FROM_ROOT".
   * But you can also specify other things like "ROOT","2D_MAX", etc
   */
  svtkSetStringMacro(OutputSelectionType);
  //@}

protected:
  svtkBoostBreadthFirstSearch();
  ~svtkBoostBreadthFirstSearch() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual int FillInputPortInformation(int port, svtkInformation* info) override;

  virtual int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkIdType OriginVertexIndex;
  char* InputArrayName;
  char* OutputArrayName;
  svtkVariant OriginValue;
  bool OutputSelection;
  bool OriginFromSelection;
  char* OutputSelectionType;

  //@{
  /**
   * Using the convenience function internally
   */
  svtkSetStringMacro(InputArrayName);
  //@}

  /**
   * This method is basically a helper function to find
   * the index of a specific value within a specific array
   */
  svtkIdType GetVertexIndex(svtkAbstractArray* abstract, svtkVariant value);

  svtkBoostBreadthFirstSearch(const svtkBoostBreadthFirstSearch&) = delete;
  void operator=(const svtkBoostBreadthFirstSearch&) = delete;
};

#endif
