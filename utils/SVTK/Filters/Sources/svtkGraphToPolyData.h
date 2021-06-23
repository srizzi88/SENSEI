/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphToPolyData.h

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
 * @class   svtkGraphToPolyData
 * @brief   convert a svtkGraph to svtkPolyData
 *
 *
 * Converts a svtkGraph to a svtkPolyData.  This assumes that the points
 * of the graph have already been filled (perhaps by svtkGraphLayout),
 * and coverts all the edge of the graph into lines in the polydata.
 * The vertex data is passed along to the point data, and the edge data
 * is passed along to the cell data.
 *
 * Only the owned graph edges (i.e. edges with ghost level 0) are copied
 * into the svtkPolyData.
 */

#ifndef svtkGraphToPolyData_h
#define svtkGraphToPolyData_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkGraphToPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkGraphToPolyData* New();
  svtkTypeMacro(svtkGraphToPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Create a second output containing points and orientation vectors
   * for drawing arrows or other glyphs on edges.  This output should be
   * set as the first input to svtkGlyph3D to place glyphs on the edges.
   * svtkGlyphSource2D's SVTK_EDGEARROW_GLYPH provides a good glyph for
   * drawing arrows.
   * Default value is off.
   */
  svtkSetMacro(EdgeGlyphOutput, bool);
  svtkGetMacro(EdgeGlyphOutput, bool);
  svtkBooleanMacro(EdgeGlyphOutput, bool);
  //@}

  //@{
  /**
   * The position of the glyph point along the edge.
   * 0 puts a glyph point at the source of each edge.
   * 1 puts a glyph point at the target of each edge.
   * An intermediate value will place the glyph point between the source and target.
   * The default value is 1.
   */
  svtkSetMacro(EdgeGlyphPosition, double);
  svtkGetMacro(EdgeGlyphPosition, double);
  //@}

protected:
  svtkGraphToPolyData();
  ~svtkGraphToPolyData() override {}

  bool EdgeGlyphOutput;
  double EdgeGlyphPosition;
  bool ArcEdges;
  svtkIdType NumberOfArcSubdivisions;

  /**
   * Convert the svtkGraph into svtkPolyData.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to svtkGraph.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGraphToPolyData(const svtkGraphToPolyData&) = delete;
  void operator=(const svtkGraphToPolyData&) = delete;
};

#endif
