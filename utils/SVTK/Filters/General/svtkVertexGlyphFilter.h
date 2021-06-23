/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVertexGlyphFilter.h

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
 * @class   svtkVertexGlyphFilter
 * @brief   Make a svtkPolyData with a vertex on each point.
 *
 *
 *
 * This filter throws away all of the cells in the input and replaces them with
 * a vertex on each point.  The intended use of this filter is roughly
 * equivalent to the svtkGlyph3D filter, except this filter is specifically for
 * data that has many vertices, making the rendered result faster and less
 * cluttered than the glyph filter. This filter may take a graph or point set
 * as input.
 *
 */

#ifndef svtkVertexGlyphFilter_h
#define svtkVertexGlyphFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkVertexGlyphFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkVertexGlyphFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkVertexGlyphFilter* New();

protected:
  svtkVertexGlyphFilter();
  ~svtkVertexGlyphFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkVertexGlyphFilter(const svtkVertexGlyphFilter&) = delete;
  void operator=(const svtkVertexGlyphFilter&) = delete;
};

#endif //_svtkVertexGlyphFilter_h
