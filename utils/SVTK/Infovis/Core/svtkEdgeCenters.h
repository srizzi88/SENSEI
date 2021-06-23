/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeCenters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEdgeCenters
 * @brief   generate points at center of edges
 *
 * svtkEdgeCenters is a filter that takes as input any graph and
 * generates on output points at the center of the cells in the dataset.
 * These points can be used for placing glyphs (svtkGlyph3D) or labeling
 * (svtkLabeledDataMapper). (The center is the parametric center of the
 * cell, not necessarily the geometric or bounding box center.) The edge
 * attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @sa
 * svtkGlyph3D svtkLabeledDataMapper
 */

#ifndef svtkEdgeCenters_h
#define svtkEdgeCenters_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkEdgeCenters : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkEdgeCenters, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static svtkEdgeCenters* New();

  //@{
  /**
   * Enable/disable the generation of vertex cells.
   */
  svtkSetMacro(VertexCells, svtkTypeBool);
  svtkGetMacro(VertexCells, svtkTypeBool);
  svtkBooleanMacro(VertexCells, svtkTypeBool);
  //@}

protected:
  svtkEdgeCenters();
  ~svtkEdgeCenters() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkTypeBool VertexCells;

private:
  svtkEdgeCenters(const svtkEdgeCenters&) = delete;
  void operator=(const svtkEdgeCenters&) = delete;
};

#endif
