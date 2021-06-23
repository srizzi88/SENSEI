/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphToGlyphs.h

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
 * @class   svtkGraphToGlyphs
 * @brief   create glyphs for graph vertices
 *
 *
 * Converts a svtkGraph to a svtkPolyData containing a glyph for each vertex.
 * This assumes that the points
 * of the graph have already been filled (perhaps by svtkGraphLayout).
 * The glyphs will automatically be scaled to be the same size in screen
 * coordinates. To do this the filter requires a pointer to the renderer
 * into which the glyphs will be rendered.
 */

#ifndef svtkGraphToGlyphs_h
#define svtkGraphToGlyphs_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSmartPointer.h"        // for SP ivars

class svtkDistanceToCamera;
class svtkGraphToPoints;
class svtkGlyph3D;
class svtkGlyphSource2D;
class svtkRenderer;
class svtkSphereSource;

class SVTKRENDERINGCORE_EXPORT svtkGraphToGlyphs : public svtkPolyDataAlgorithm
{
public:
  static svtkGraphToGlyphs* New();
  svtkTypeMacro(svtkGraphToGlyphs, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    VERTEX = 1,
    DASH,
    CROSS,
    THICKCROSS,
    TRIANGLE,
    SQUARE,
    CIRCLE,
    DIAMOND,
    SPHERE
  };

  //@{
  /**
   * The glyph type, specified as one of the enumerated values in this
   * class. VERTEX is a special glyph that cannot be scaled, but instead
   * is rendered as an OpenGL vertex primitive. This may appear as a box
   * or circle depending on the hardware.
   */
  svtkSetMacro(GlyphType, int);
  svtkGetMacro(GlyphType, int);
  //@}

  //@{
  /**
   * Whether to fill the glyph, or to just render the outline.
   */
  svtkSetMacro(Filled, bool);
  svtkGetMacro(Filled, bool);
  svtkBooleanMacro(Filled, bool);
  //@}

  //@{
  /**
   * Set the desired screen size of each glyph. If you are using scaling,
   * this will be the size of the glyph when rendering an object with
   * scaling value 1.0.
   */
  svtkSetMacro(ScreenSize, double);
  svtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * The renderer in which the glyphs will be placed.
   */
  virtual void SetRenderer(svtkRenderer* ren);
  virtual svtkRenderer* GetRenderer();
  //@}

  //@{
  /**
   * Whether to use the input array to process in order to scale the
   * vertices.
   */
  virtual void SetScaling(bool b);
  virtual bool GetScaling();
  //@}

  /**
   * The modified time of this filter.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkGraphToGlyphs();
  ~svtkGraphToGlyphs() override;

  /**
   * Convert the svtkGraph into svtkPolyData.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to svtkGraph.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkSmartPointer<svtkGraphToPoints> GraphToPoints;
  svtkSmartPointer<svtkGlyphSource2D> GlyphSource;
  svtkSmartPointer<svtkSphereSource> Sphere;
  svtkSmartPointer<svtkGlyph3D> Glyph;
  svtkSmartPointer<svtkDistanceToCamera> DistanceToCamera;
  int GlyphType;
  bool Filled;
  double ScreenSize;

private:
  svtkGraphToGlyphs(const svtkGraphToGlyphs&) = delete;
  void operator=(const svtkGraphToGlyphs&) = delete;
};

#endif
