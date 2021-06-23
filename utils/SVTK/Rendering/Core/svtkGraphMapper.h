/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphMapper.h

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
 * @class   svtkGraphMapper
 * @brief   map svtkGraph and derived
 * classes to graphics primitives
 *
 *
 * svtkGraphMapper is a mapper to map svtkGraph
 * (and all derived classes) to graphics primitives.
 */

#ifndef svtkGraphMapper_h
#define svtkGraphMapper_h

#include "svtkMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkSmartPointer.h" // Required for smart pointer internal ivars.

class svtkActor2D;
class svtkMapArrayValues;
class svtkCamera;
class svtkFollower;
class svtkGraph;
class svtkGlyph3D;
class svtkGraphToPolyData;
class svtkIconGlyphFilter;
class svtkCellCenters;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkPolyDataMapper2D;
class svtkLookupTable;
class svtkTransformCoordinateSystems;
class svtkTexture;
class svtkTexturedActor2D;
class svtkVertexGlyphFilter;

class SVTKRENDERINGCORE_EXPORT svtkGraphMapper : public svtkMapper
{
public:
  static svtkGraphMapper* New();
  svtkTypeMacro(svtkGraphMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  void Render(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * The array to use for coloring vertices.  Default is "color".
   */
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  void ColorVerticesOn();
  void ColorVerticesOff();
  //@}

  //@{
  /**
   * Whether scaled glyphs are on or not.  Default is off.
   * By default this mapper uses vertex glyphs that do not
   * scale. If you turn this option on you will get circles
   * at each vertex and they will scale as you zoom in/out.
   */
  void SetScaledGlyphs(bool arg);
  svtkGetMacro(ScaledGlyphs, bool);
  svtkBooleanMacro(ScaledGlyphs, bool);
  //@}

  //@{
  /**
   * Glyph scaling array name. Default is "scale"
   */
  svtkSetStringMacro(ScalingArrayName);
  svtkGetStringMacro(ScalingArrayName);
  //@}

  //@{
  /**
   * Whether to show edges or not.  Default is on.
   */
  void SetEdgeVisibility(bool vis);
  bool GetEdgeVisibility();
  svtkBooleanMacro(EdgeVisibility, bool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  //@}

  //@{
  /**
   * Whether to color edges.  Default is off.
   */
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  svtkSetStringMacro(EnabledEdgesArrayName);
  svtkGetStringMacro(EnabledEdgesArrayName);
  //@}

  //@{
  /**
   * Whether to enable/disable edges using array values.  Default is off.
   */
  svtkSetMacro(EnableEdgesByArray, svtkTypeBool);
  svtkGetMacro(EnableEdgesByArray, svtkTypeBool);
  svtkBooleanMacro(EnableEdgesByArray, svtkTypeBool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  svtkSetStringMacro(EnabledVerticesArrayName);
  svtkGetStringMacro(EnabledVerticesArrayName);
  //@}

  //@{
  /**
   * Whether to enable/disable vertices using array values.  Default is off.
   */
  svtkSetMacro(EnableVerticesByArray, svtkTypeBool);
  svtkGetMacro(EnableVerticesByArray, svtkTypeBool);
  svtkBooleanMacro(EnableVerticesByArray, svtkTypeBool);
  //@}

  //@{
  /**
   * The array to use for assigning icons.
   */
  void SetIconArrayName(const char* name);
  const char* GetIconArrayName();
  //@}

  /**
   * Associate the icon at index "index" in the svtkTexture to all vertices
   * containing "type" as a value in the vertex attribute array specified by
   * IconArrayName.
   */
  void AddIconType(const char* type, int index);

  /**
   * Clear all icon mappings.
   */
  void ClearIconTypes();

  //@{
  /**
   * Specify the Width and Height, in pixels, of an icon in the icon sheet.
   */
  void SetIconSize(int* size);
  int* GetIconSize();
  //@}

  /**
   * Specify where the icons should be placed in relation to the vertex.
   * See svtkIconGlyphFilter.h for possible values.
   */
  void SetIconAlignment(int alignment);

  //@{
  /**
   * The texture containing the icon sheet.
   */
  svtkTexture* GetIconTexture();
  void SetIconTexture(svtkTexture* texture);
  //@}

  //@{
  /**
   * Whether to show icons.  Default is off.
   */
  void SetIconVisibility(bool vis);
  bool GetIconVisibility();
  svtkBooleanMacro(IconVisibility, bool);
  //@}

  //@{
  /**
   * Get/Set the vertex point size
   */
  svtkGetMacro(VertexPointSize, float);
  void SetVertexPointSize(float size);
  //@}

  //@{
  /**
   * Get/Set the edge line width
   */
  svtkGetMacro(EdgeLineWidth, float);
  void SetEdgeLineWidth(float width);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Get the mtime also considering the lookup table.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the Input of this mapper.
   */
  void SetInputData(svtkGraph* input);
  svtkGraph* GetInput();
  //@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(double* bounds) override { Superclass::GetBounds(bounds); }

  //@{
  /**
   * Access to the lookup tables used by the vertex and edge mappers.
   */
  svtkGetObjectMacro(EdgeLookupTable, svtkLookupTable);
  svtkGetObjectMacro(VertexLookupTable, svtkLookupTable);
  //@}

protected:
  svtkGraphMapper();
  ~svtkGraphMapper() override;

  //@{
  /**
   * Used to store the vertex and edge color array names
   */
  svtkGetStringMacro(VertexColorArrayNameInternal);
  svtkSetStringMacro(VertexColorArrayNameInternal);
  svtkGetStringMacro(EdgeColorArrayNameInternal);
  svtkSetStringMacro(EdgeColorArrayNameInternal);
  char* VertexColorArrayNameInternal;
  char* EdgeColorArrayNameInternal;
  //@}

  char* EnabledEdgesArrayName;
  char* EnabledVerticesArrayName;
  svtkTypeBool EnableEdgesByArray;
  svtkTypeBool EnableVerticesByArray;

  svtkGetStringMacro(IconArrayNameInternal);
  svtkSetStringMacro(IconArrayNameInternal);
  char* IconArrayNameInternal;

  svtkSmartPointer<svtkGlyph3D> CircleGlyph;
  svtkSmartPointer<svtkGlyph3D> CircleOutlineGlyph;

  svtkSmartPointer<svtkGraphToPolyData> GraphToPoly;
  svtkSmartPointer<svtkVertexGlyphFilter> VertexGlyph;
  svtkSmartPointer<svtkIconGlyphFilter> IconGlyph;
  svtkSmartPointer<svtkMapArrayValues> IconTypeToIndex;
  svtkSmartPointer<svtkTransformCoordinateSystems> IconTransform;

  svtkSmartPointer<svtkPolyDataMapper> EdgeMapper;
  svtkSmartPointer<svtkPolyDataMapper> VertexMapper;
  svtkSmartPointer<svtkPolyDataMapper> OutlineMapper;
  svtkSmartPointer<svtkPolyDataMapper2D> IconMapper;

  svtkSmartPointer<svtkActor> EdgeActor;
  svtkSmartPointer<svtkActor> VertexActor;
  svtkSmartPointer<svtkActor> OutlineActor;
  svtkSmartPointer<svtkTexturedActor2D> IconActor;

  // Color maps
  svtkLookupTable* EdgeLookupTable;
  svtkLookupTable* VertexLookupTable;

  void ReportReferences(svtkGarbageCollector*) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGraphMapper(const svtkGraphMapper&) = delete;
  void operator=(const svtkGraphMapper&) = delete;

  // Helper function
  svtkPolyData* CreateCircle(bool filled);

  float VertexPointSize;
  float EdgeLineWidth;
  bool ScaledGlyphs;
  char* ScalingArrayName;
};

#endif
