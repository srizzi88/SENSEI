/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedGraphRepresentation.h

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
 * @class   svtkRenderedGraphRepresentation
 *
 *
 */

#ifndef svtkRenderedGraphRepresentation_h
#define svtkRenderedGraphRepresentation_h

#include "svtkRenderedRepresentation.h"
#include "svtkSmartPointer.h"       // for SP ivars
#include "svtkViewsInfovisModule.h" // For export macro

class svtkActor;
class svtkApplyColors;
class svtkApplyIcons;
class svtkEdgeCenters;
class svtkEdgeLayout;
class svtkEdgeLayoutStrategy;
class svtkGraphLayout;
class svtkGraphLayoutStrategy;
class svtkGraphToGlyphs;
class svtkGraphToPoints;
class svtkGraphToPolyData;
class svtkIconGlyphFilter;
class svtkInformation;
class svtkInformationVector;
class svtkLookupTable;
class svtkPerturbCoincidentVertices;
class svtkPointSetToLabelHierarchy;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkPolyDataMapper2D;
class svtkRemoveHiddenData;
class svtkRenderView;
class svtkScalarBarWidget;
class svtkScalarsToColors;
class svtkTextProperty;
class svtkTexturedActor2D;
class svtkTransformCoordinateSystems;
class svtkVertexDegree;
class svtkView;
class svtkViewTheme;

class SVTKVIEWSINFOVIS_EXPORT svtkRenderedGraphRepresentation : public svtkRenderedRepresentation
{
public:
  static svtkRenderedGraphRepresentation* New();
  svtkTypeMacro(svtkRenderedGraphRepresentation, svtkRenderedRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // ------------------------------------------------------------------------
  // Vertex labels

  virtual void SetVertexLabelArrayName(const char* name);
  virtual const char* GetVertexLabelArrayName();
  virtual void SetVertexLabelPriorityArrayName(const char* name);
  virtual const char* GetVertexLabelPriorityArrayName();
  virtual void SetVertexLabelVisibility(bool b);
  virtual bool GetVertexLabelVisibility();
  svtkBooleanMacro(VertexLabelVisibility, bool);
  virtual void SetVertexLabelTextProperty(svtkTextProperty* p);
  virtual svtkTextProperty* GetVertexLabelTextProperty();
  svtkSetStringMacro(VertexHoverArrayName);
  svtkGetStringMacro(VertexHoverArrayName);
  //@{
  /**
   * Whether to hide the display of vertex labels during mouse interaction.  Default is off.
   */
  svtkSetMacro(HideVertexLabelsOnInteraction, bool);
  svtkGetMacro(HideVertexLabelsOnInteraction, bool);
  svtkBooleanMacro(HideVertexLabelsOnInteraction, bool);
  //@}

  // ------------------------------------------------------------------------
  // Edge labels

  virtual void SetEdgeLabelArrayName(const char* name);
  virtual const char* GetEdgeLabelArrayName();
  virtual void SetEdgeLabelPriorityArrayName(const char* name);
  virtual const char* GetEdgeLabelPriorityArrayName();
  virtual void SetEdgeLabelVisibility(bool b);
  virtual bool GetEdgeLabelVisibility();
  svtkBooleanMacro(EdgeLabelVisibility, bool);
  virtual void SetEdgeLabelTextProperty(svtkTextProperty* p);
  virtual svtkTextProperty* GetEdgeLabelTextProperty();
  svtkSetStringMacro(EdgeHoverArrayName);
  svtkGetStringMacro(EdgeHoverArrayName);
  //@{
  /**
   * Whether to hide the display of edge labels during mouse interaction.  Default is off.
   */
  svtkSetMacro(HideEdgeLabelsOnInteraction, bool);
  svtkGetMacro(HideEdgeLabelsOnInteraction, bool);
  svtkBooleanMacro(HideEdgeLabelsOnInteraction, bool);
  //@}

  // ------------------------------------------------------------------------
  // Vertex icons

  virtual void SetVertexIconArrayName(const char* name);
  virtual const char* GetVertexIconArrayName();
  virtual void SetVertexIconPriorityArrayName(const char* name);
  virtual const char* GetVertexIconPriorityArrayName();
  virtual void SetVertexIconVisibility(bool b);
  virtual bool GetVertexIconVisibility();
  svtkBooleanMacro(VertexIconVisibility, bool);
  virtual void AddVertexIconType(const char* name, int type);
  virtual void ClearVertexIconTypes();
  virtual void SetUseVertexIconTypeMap(bool b);
  virtual bool GetUseVertexIconTypeMap();
  svtkBooleanMacro(UseVertexIconTypeMap, bool);
  virtual void SetVertexIconAlignment(int align);
  virtual int GetVertexIconAlignment();
  virtual void SetVertexSelectedIcon(int icon);
  virtual int GetVertexSelectedIcon();
  virtual void SetVertexDefaultIcon(int icon);
  virtual int GetVertexDefaultIcon();

  //@{
  /**
   * Set the mode to one of
   * <ul>
   * <li>svtkApplyIcons::SELECTED_ICON - use VertexSelectedIcon
   * <li>svtkApplyIcons::SELECTED_OFFSET - use VertexSelectedIcon as offset
   * <li>svtkApplyIcons::ANNOTATION_ICON - use current annotation icon
   * <li>svtkApplyIcons::IGNORE_SELECTION - ignore selected elements
   * </ul>
   * The default is IGNORE_SELECTION.
   */
  virtual void SetVertexIconSelectionMode(int mode);
  virtual int GetVertexIconSelectionMode();
  virtual void SetVertexIconSelectionModeToSelectedIcon() { this->SetVertexIconSelectionMode(0); }
  virtual void SetVertexIconSelectionModeToSelectedOffset() { this->SetVertexIconSelectionMode(1); }
  virtual void SetVertexIconSelectionModeToAnnotationIcon() { this->SetVertexIconSelectionMode(2); }
  virtual void SetVertexIconSelectionModeToIgnoreSelection()
  {
    this->SetVertexIconSelectionMode(3);
  }
  //@}

  // ------------------------------------------------------------------------
  // Edge icons

  virtual void SetEdgeIconArrayName(const char* name);
  virtual const char* GetEdgeIconArrayName();
  virtual void SetEdgeIconPriorityArrayName(const char* name);
  virtual const char* GetEdgeIconPriorityArrayName();
  virtual void SetEdgeIconVisibility(bool b);
  virtual bool GetEdgeIconVisibility();
  svtkBooleanMacro(EdgeIconVisibility, bool);
  virtual void AddEdgeIconType(const char* name, int type);
  virtual void ClearEdgeIconTypes();
  virtual void SetUseEdgeIconTypeMap(bool b);
  virtual bool GetUseEdgeIconTypeMap();
  svtkBooleanMacro(UseEdgeIconTypeMap, bool);
  virtual void SetEdgeIconAlignment(int align);
  virtual int GetEdgeIconAlignment();

  // ------------------------------------------------------------------------
  // Vertex colors

  virtual void SetColorVerticesByArray(bool b);
  virtual bool GetColorVerticesByArray();
  svtkBooleanMacro(ColorVerticesByArray, bool);
  virtual void SetVertexColorArrayName(const char* name);
  virtual const char* GetVertexColorArrayName();

  // ------------------------------------------------------------------------
  // Edge colors

  virtual void SetColorEdgesByArray(bool b);
  virtual bool GetColorEdgesByArray();
  svtkBooleanMacro(ColorEdgesByArray, bool);
  virtual void SetEdgeColorArrayName(const char* name);
  virtual const char* GetEdgeColorArrayName();

  // ------------------------------------------------------------------------
  // Enabled vertices

  virtual void SetEnableVerticesByArray(bool b);
  virtual bool GetEnableVerticesByArray();
  svtkBooleanMacro(EnableVerticesByArray, bool);
  virtual void SetEnabledVerticesArrayName(const char* name);
  virtual const char* GetEnabledVerticesArrayName();

  // ------------------------------------------------------------------------
  // Enabled edges

  virtual void SetEnableEdgesByArray(bool b);
  virtual bool GetEnableEdgesByArray();
  svtkBooleanMacro(EnableEdgesByArray, bool);
  virtual void SetEnabledEdgesArrayName(const char* name);
  virtual const char* GetEnabledEdgesArrayName();

  virtual void SetEdgeVisibility(bool b);
  virtual bool GetEdgeVisibility();
  svtkBooleanMacro(EdgeVisibility, bool);

  void SetEdgeSelection(bool b);
  bool GetEdgeSelection();

  // ------------------------------------------------------------------------
  // Vertex layout strategy

  //@{
  /**
   * Set/get the graph layout strategy.
   */
  virtual void SetLayoutStrategy(svtkGraphLayoutStrategy* strategy);
  virtual svtkGraphLayoutStrategy* GetLayoutStrategy();
  //@}

  //@{
  /**
   * Get/set the layout strategy by name.
   */
  virtual void SetLayoutStrategy(const char* name);
  svtkGetStringMacro(LayoutStrategyName);
  //@}

  /**
   * Set predefined layout strategies.
   */
  void SetLayoutStrategyToRandom() { this->SetLayoutStrategy("Random"); }
  void SetLayoutStrategyToForceDirected() { this->SetLayoutStrategy("Force Directed"); }
  void SetLayoutStrategyToSimple2D() { this->SetLayoutStrategy("Simple 2D"); }
  void SetLayoutStrategyToClustering2D() { this->SetLayoutStrategy("Clustering 2D"); }
  void SetLayoutStrategyToCommunity2D() { this->SetLayoutStrategy("Community 2D"); }
  void SetLayoutStrategyToFast2D() { this->SetLayoutStrategy("Fast 2D"); }
  void SetLayoutStrategyToPassThrough() { this->SetLayoutStrategy("Pass Through"); }
  void SetLayoutStrategyToCircular() { this->SetLayoutStrategy("Circular"); }
  void SetLayoutStrategyToTree() { this->SetLayoutStrategy("Tree"); }
  void SetLayoutStrategyToCosmicTree() { this->SetLayoutStrategy("Cosmic Tree"); }
  void SetLayoutStrategyToCone() { this->SetLayoutStrategy("Cone"); }
  void SetLayoutStrategyToSpanTree() { this->SetLayoutStrategy("Span Tree"); }

  /**
   * Set the layout strategy to use coordinates from arrays.
   * The x array must be specified. The y and z arrays are optional.
   */
  virtual void SetLayoutStrategyToAssignCoordinates(
    const char* xarr, const char* yarr = nullptr, const char* zarr = nullptr);

  /**
   * Set the layout strategy to a tree layout. Radial indicates whether to
   * do a radial or standard top-down tree layout. The angle parameter is the
   * angular distance spanned by the tree. Leaf spacing is a
   * value from 0 to 1 indicating how much of the radial layout should be
   * allocated to leaf nodes (as opposed to between tree branches). The log spacing value is a
   * non-negative value where > 1 will create expanding levels, < 1 will create
   * contracting levels, and = 1 makes all levels the same size. See
   * svtkTreeLayoutStrategy for more information.
   */
  virtual void SetLayoutStrategyToTree(
    bool radial, double angle = 90, double leafSpacing = 0.9, double logSpacing = 1.0);

  /**
   * Set the layout strategy to a cosmic tree layout. nodeSizeArrayName is
   * the array used to size the circles (default is nullptr, which makes leaf
   * nodes the same size). sizeLeafNodesOnly only uses the leaf node sizes,
   * and computes the parent size as the sum of the child sizes (default true).
   * layoutDepth stops layout at a certain depth (default is 0, which does the
   * entire tree). layoutRoot is the vertex that will be considered the root
   * node of the layout (default is -1, which will use the tree's root).
   * See svtkCosmicTreeLayoutStrategy for more information.
   */
  virtual void SetLayoutStrategyToCosmicTree(const char* nodeSizeArrayName,
    bool sizeLeafNodesOnly = true, int layoutDepth = 0, svtkIdType layoutRoot = -1);

  // ------------------------------------------------------------------------
  // Edge layout strategy

  //@{
  /**
   * Set/get the graph layout strategy.
   */
  virtual void SetEdgeLayoutStrategy(svtkEdgeLayoutStrategy* strategy);
  virtual svtkEdgeLayoutStrategy* GetEdgeLayoutStrategy();
  void SetEdgeLayoutStrategyToArcParallel() { this->SetEdgeLayoutStrategy("Arc Parallel"); }
  void SetEdgeLayoutStrategyToPassThrough() { this->SetEdgeLayoutStrategy("Pass Through"); }
  //@}

  /**
   * Set the edge layout strategy to a geospatial arced strategy
   * appropriate for svtkGeoView.
   */
  virtual void SetEdgeLayoutStrategyToGeo(double explodeFactor = 0.2);

  //@{
  /**
   * Set the edge layout strategy by name.
   */
  virtual void SetEdgeLayoutStrategy(const char* name);
  svtkGetStringMacro(EdgeLayoutStrategyName);
  //@}

  // ------------------------------------------------------------------------
  // Miscellaneous

  /**
   * Apply a theme to this representation.
   */
  void ApplyViewTheme(svtkViewTheme* theme) override;

  //@{
  /**
   * Set the graph vertex glyph type.
   */
  virtual void SetGlyphType(int type);
  virtual int GetGlyphType();
  //@}

  //@{
  /**
   * Set whether to scale vertex glyphs.
   */
  virtual void SetScaling(bool b);
  virtual bool GetScaling();
  svtkBooleanMacro(Scaling, bool);
  //@}

  //@{
  /**
   * Set the glyph scaling array name.
   */
  virtual void SetScalingArrayName(const char* name);
  virtual const char* GetScalingArrayName();
  //@}

  //@{
  /**
   * Vertex/edge scalar bar visibility.
   */
  virtual void SetVertexScalarBarVisibility(bool b);
  virtual bool GetVertexScalarBarVisibility();
  virtual void SetEdgeScalarBarVisibility(bool b);
  virtual bool GetEdgeScalarBarVisibility();
  //@}

  //@{
  /**
   * Obtain the scalar bar widget used to draw a legend for the vertices/edges.
   */
  virtual svtkScalarBarWidget* GetVertexScalarBar();
  virtual svtkScalarBarWidget* GetEdgeScalarBar();
  //@}

  /**
   * Whether the current graph layout is complete.
   */
  virtual bool IsLayoutComplete();

  /**
   * Performs another iteration on the graph layout.
   */
  virtual void UpdateLayout();

  /**
   * Compute the bounding box of the selected subgraph.
   */
  void ComputeSelectedGraphBounds(double bounds[6]);

protected:
  svtkRenderedGraphRepresentation();
  ~svtkRenderedGraphRepresentation() override;

  //@{
  /**
   * Called by the view to add/remove this representation.
   */
  bool AddToView(svtkView* view) override;
  bool RemoveFromView(svtkView* view) override;
  //@}

  void PrepareForRendering(svtkRenderView* view) override;

  svtkSelection* ConvertSelection(svtkView* view, svtkSelection* sel) override;

  svtkUnicodeString GetHoverTextInternal(svtkSelection* sel) override;

  /**
   * Connect inputs to internal pipeline.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //@{
  /**
   * Internal filter classes.
   */
  svtkSmartPointer<svtkApplyColors> ApplyColors;
  svtkSmartPointer<svtkVertexDegree> VertexDegree;
  svtkSmartPointer<svtkPolyData> EmptyPolyData;
  svtkSmartPointer<svtkEdgeCenters> EdgeCenters;
  svtkSmartPointer<svtkGraphToPoints> GraphToPoints;
  svtkSmartPointer<svtkPointSetToLabelHierarchy> VertexLabelHierarchy;
  svtkSmartPointer<svtkPointSetToLabelHierarchy> EdgeLabelHierarchy;
  svtkSmartPointer<svtkGraphLayout> Layout;
  svtkSmartPointer<svtkPerturbCoincidentVertices> Coincident;
  svtkSmartPointer<svtkEdgeLayout> EdgeLayout;
  svtkSmartPointer<svtkGraphToPolyData> GraphToPoly;
  svtkSmartPointer<svtkPolyDataMapper> EdgeMapper;
  svtkSmartPointer<svtkActor> EdgeActor;
  svtkSmartPointer<svtkGraphToGlyphs> VertexGlyph;
  svtkSmartPointer<svtkPolyDataMapper> VertexMapper;
  svtkSmartPointer<svtkActor> VertexActor;
  svtkSmartPointer<svtkGraphToGlyphs> OutlineGlyph;
  svtkSmartPointer<svtkPolyDataMapper> OutlineMapper;
  svtkSmartPointer<svtkActor> OutlineActor;
  svtkSmartPointer<svtkScalarBarWidget> VertexScalarBar;
  svtkSmartPointer<svtkScalarBarWidget> EdgeScalarBar;
  svtkSmartPointer<svtkRemoveHiddenData> RemoveHiddenGraph;
  svtkSmartPointer<svtkApplyIcons> ApplyVertexIcons;
  svtkSmartPointer<svtkGraphToPoints> VertexIconPoints;
  svtkSmartPointer<svtkTransformCoordinateSystems> VertexIconTransform;
  svtkSmartPointer<svtkIconGlyphFilter> VertexIconGlyph;
  svtkSmartPointer<svtkPolyDataMapper2D> VertexIconMapper;
  svtkSmartPointer<svtkTexturedActor2D> VertexIconActor;
  //@}

  char* VertexHoverArrayName;
  char* EdgeHoverArrayName;

  svtkSetStringMacro(VertexColorArrayNameInternal);
  svtkGetStringMacro(VertexColorArrayNameInternal);
  char* VertexColorArrayNameInternal;

  svtkSetStringMacro(EdgeColorArrayNameInternal);
  svtkGetStringMacro(EdgeColorArrayNameInternal);
  char* EdgeColorArrayNameInternal;

  svtkSetStringMacro(ScalingArrayNameInternal);
  svtkGetStringMacro(ScalingArrayNameInternal);
  char* ScalingArrayNameInternal;

  svtkSetStringMacro(LayoutStrategyName);
  char* LayoutStrategyName;
  svtkSetStringMacro(EdgeLayoutStrategyName);
  char* EdgeLayoutStrategyName;
  bool HideVertexLabelsOnInteraction;
  bool HideEdgeLabelsOnInteraction;

  bool EdgeSelection;

private:
  svtkRenderedGraphRepresentation(const svtkRenderedGraphRepresentation&) = delete;
  void operator=(const svtkRenderedGraphRepresentation&) = delete;
};

#endif
