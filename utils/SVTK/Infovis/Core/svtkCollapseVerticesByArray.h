/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollapseVerticesByArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCollapseVerticesByArray
 * @brief   Collapse the graph given a vertex array
 *
 *
 * svtkCollapseVerticesByArray is a class which collapses the graph using
 * a vertex array as the key. So if the graph has vertices sharing common
 * traits then this class combines all these vertices into one. This class
 * does not perform aggregation on vertex data but allow to do so for edge data.
 * Users can choose one or more edge data arrays for aggregation using
 * AddAggregateEdgeArray function.
 *
 *
 */

#ifndef svtkCollapseVerticesByArray_h
#define svtkCollapseVerticesByArray_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkCollapseVerticesByArrayInternal;

class SVTKINFOVISCORE_EXPORT svtkCollapseVerticesByArray : public svtkGraphAlgorithm
{
public:
  static svtkCollapseVerticesByArray* New();
  svtkTypeMacro(svtkCollapseVerticesByArray, svtkGraphAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Boolean to allow self loops during collapse.
   */
  svtkGetMacro(AllowSelfLoops, bool);
  svtkSetMacro(AllowSelfLoops, bool);
  svtkBooleanMacro(AllowSelfLoops, bool);
  //@}

  /**
   * Add arrays on which aggregation of data is allowed.
   * Default if replaced by the last value.
   */
  void AddAggregateEdgeArray(const char* arrName);

  /**
   * Clear the list of arrays on which aggregation was set to allow.
   */
  void ClearAggregateEdgeArray();

  //@{
  /**
   * Set the array using which perform the collapse.
   */
  svtkGetStringMacro(VertexArray);
  svtkSetStringMacro(VertexArray);
  //@}

  //@{
  /**
   * Set if count should be made of how many edges collapsed.
   */
  svtkGetMacro(CountEdgesCollapsed, bool);
  svtkSetMacro(CountEdgesCollapsed, bool);
  svtkBooleanMacro(CountEdgesCollapsed, bool);
  //@}

  //@{
  /**
   * Name of the array where the count of how many edges collapsed will
   * be stored.By default the name of array is "EdgesCollapsedCountArray".
   */
  svtkGetStringMacro(EdgesCollapsedArray);
  svtkSetStringMacro(EdgesCollapsedArray);
  //@}

  //@{
  /**
   * Get/Set if count should be made of how many vertices collapsed.
   */
  svtkGetMacro(CountVerticesCollapsed, bool);
  svtkSetMacro(CountVerticesCollapsed, bool);
  svtkBooleanMacro(CountVerticesCollapsed, bool);
  //@}

  //@{
  /**
   * Name of the array where the count of how many vertices collapsed will
   * be stored. By default name of the array is "VerticesCollapsedCountArray".
   */
  svtkGetStringMacro(VerticesCollapsedArray);
  svtkSetStringMacro(VerticesCollapsedArray);
  //@}

protected:
  svtkCollapseVerticesByArray();
  ~svtkCollapseVerticesByArray() override;

  /**
   * Pipeline function.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Pipeline function.
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Create output graph given all the parameters. Helper function.
   */
  svtkGraph* Create(svtkGraph* inGraph);

  /**
   * Helper function.
   */
  void FindEdge(svtkGraph* outGraph, svtkIdType source, svtkIdType target, svtkIdType& edgeId);

private:
  //@{
  svtkCollapseVerticesByArray(const svtkCollapseVerticesByArray&) = delete;
  void operator=(const svtkCollapseVerticesByArray&) = delete;
  //@}

protected:
  bool AllowSelfLoops;
  char* VertexArray;

  bool CountEdgesCollapsed;
  char* EdgesCollapsedArray;

  bool CountVerticesCollapsed;
  char* VerticesCollapsedArray;

  svtkCollapseVerticesByArrayInternal* Internal;
};

#endif // svtkCollapseVerticesByArray_h__
