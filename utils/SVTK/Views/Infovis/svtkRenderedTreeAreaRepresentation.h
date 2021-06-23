/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedTreeAreaRepresentation.h

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
 * @class   svtkRenderedTreeAreaRepresentation
 *
 *
 */

#ifndef svtkRenderedTreeAreaRepresentation_h
#define svtkRenderedTreeAreaRepresentation_h

#include "svtkRenderedRepresentation.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkActor;
class svtkActor2D;
class svtkAreaLayout;
class svtkAreaLayoutStrategy;
class svtkConvertSelection;
class svtkEdgeCenters;
class svtkExtractSelectedPolyDataIds;
class svtkLabeledDataMapper;
class svtkPointSetToLabelHierarchy;
class svtkPolyData;
class svtkPolyDataAlgorithm;
class svtkPolyDataMapper;
class svtkScalarBarWidget;
class svtkTextProperty;
class svtkTreeFieldAggregator;
class svtkTreeLevelsFilter;
class svtkVertexDegree;
class svtkWorldPointPicker;

class SVTKVIEWSINFOVIS_EXPORT svtkRenderedTreeAreaRepresentation : public svtkRenderedRepresentation
{
public:
  static svtkRenderedTreeAreaRepresentation* New();
  svtkTypeMacro(svtkRenderedTreeAreaRepresentation, svtkRenderedRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the label render mode.
   * QT - Use svtkQtTreeRingLabeler with fitted labeling
   * and unicode support. Requires SVTK_USE_QT to be on.
   * FREETYPE - Use standard freetype text rendering.
   */
  void SetLabelRenderMode(int mode) override;

  //@{
  /**
   * The array to use for area labeling.  Default is "label".
   */
  virtual void SetAreaLabelArrayName(const char* name);
  virtual const char* GetAreaLabelArrayName();
  //@}

  //@{
  /**
   * The array to use for area sizes. Default is "size".
   */
  virtual void SetAreaSizeArrayName(const char* name);
  virtual const char* GetAreaSizeArrayName();
  //@}

  //@{
  /**
   * The array to use for area labeling priority.
   * Default is "GraphVertexDegree".
   */
  virtual void SetAreaLabelPriorityArrayName(const char* name);
  virtual const char* GetAreaLabelPriorityArrayName();
  //@}

  //@{
  /**
   * The array to use for edge labeling.  Default is "label".
   */
  virtual void SetGraphEdgeLabelArrayName(const char* name)
  {
    this->SetGraphEdgeLabelArrayName(name, 0);
  }
  virtual void SetGraphEdgeLabelArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeLabelArrayName() { return this->GetGraphEdgeLabelArrayName(0); }
  virtual const char* GetGraphEdgeLabelArrayName(int idx);
  //@}

  //@{
  /**
   * The text property for the graph edge labels.
   */
  virtual void SetGraphEdgeLabelTextProperty(svtkTextProperty* tp)
  {
    this->SetGraphEdgeLabelTextProperty(tp, 0);
  }
  virtual void SetGraphEdgeLabelTextProperty(svtkTextProperty* tp, int idx);
  virtual svtkTextProperty* GetGraphEdgeLabelTextProperty()
  {
    return this->GetGraphEdgeLabelTextProperty(0);
  }
  virtual svtkTextProperty* GetGraphEdgeLabelTextProperty(int idx);
  //@}

  //@{
  /**
   * The name of the array whose value appears when the mouse hovers
   * over a rectangle in the treemap.
   */
  svtkSetStringMacro(AreaHoverArrayName);
  svtkGetStringMacro(AreaHoverArrayName);
  //@}

  //@{
  /**
   * Whether to show area labels.  Default is off.
   */
  virtual void SetAreaLabelVisibility(bool vis);
  virtual bool GetAreaLabelVisibility();
  svtkBooleanMacro(AreaLabelVisibility, bool);
  //@}

  //@{
  /**
   * The text property for the area labels.
   */
  virtual void SetAreaLabelTextProperty(svtkTextProperty* tp);
  virtual svtkTextProperty* GetAreaLabelTextProperty();
  //@}

  //@{
  /**
   * Whether to show edge labels.  Default is off.
   */
  virtual void SetGraphEdgeLabelVisibility(bool vis) { this->SetGraphEdgeLabelVisibility(vis, 0); }
  virtual void SetGraphEdgeLabelVisibility(bool vis, int idx);
  virtual bool GetGraphEdgeLabelVisibility() { return this->GetGraphEdgeLabelVisibility(0); }
  virtual bool GetGraphEdgeLabelVisibility(int idx);
  svtkBooleanMacro(GraphEdgeLabelVisibility, bool);
  //@}

  //@{
  /**
   * The array to use for coloring vertices.  Default is "color".
   */
  void SetAreaColorArrayName(const char* name);
  const char* GetAreaColorArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  virtual void SetColorAreasByArray(bool vis);
  virtual bool GetColorAreasByArray();
  svtkBooleanMacro(ColorAreasByArray, bool);
  //@}

  //@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  virtual void SetGraphEdgeColorArrayName(const char* name)
  {
    this->SetGraphEdgeColorArrayName(name, 0);
  }
  virtual void SetGraphEdgeColorArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeColorArrayName() { return this->GetGraphEdgeColorArrayName(0); }
  virtual const char* GetGraphEdgeColorArrayName(int idx);
  //@}

  /**
   * Set the color to be the spline fraction
   */
  virtual void SetGraphEdgeColorToSplineFraction() { this->SetGraphEdgeColorToSplineFraction(0); }
  virtual void SetGraphEdgeColorToSplineFraction(int idx);

  //@{
  /**
   * Whether to color edges.  Default is off.
   */
  virtual void SetColorGraphEdgesByArray(bool vis) { this->SetColorGraphEdgesByArray(vis, 0); }
  virtual void SetColorGraphEdgesByArray(bool vis, int idx);
  virtual bool GetColorGraphEdgesByArray() { return this->GetColorGraphEdgesByArray(0); }
  virtual bool GetColorGraphEdgesByArray(int idx);
  svtkBooleanMacro(ColorGraphEdgesByArray, bool);
  //@}

  //@{
  /**
   * The name of the array whose value appears when the mouse hovers
   * over a graph edge.
   */
  virtual void SetGraphHoverArrayName(const char* name) { this->SetGraphHoverArrayName(name, 0); }
  virtual void SetGraphHoverArrayName(const char* name, int idx);
  virtual const char* GetGraphHoverArrayName() { return this->GetGraphHoverArrayName(0); }
  virtual const char* GetGraphHoverArrayName(int idx);
  //@}

  //@{
  /**
   * Set the region shrink percentage between 0.0 and 1.0.
   */
  virtual void SetShrinkPercentage(double value);
  virtual double GetShrinkPercentage();
  //@}

  //@{
  /**
   * Set the bundling strength.
   */
  virtual void SetGraphBundlingStrength(double strength)
  {
    this->SetGraphBundlingStrength(strength, 0);
  }
  virtual void SetGraphBundlingStrength(double strength, int idx);
  virtual double GetGraphBundlingStrength() { return this->GetGraphBundlingStrength(0); }
  virtual double GetGraphBundlingStrength(int idx);
  //@}

  //@{
  /**
   * Sets the spline type for the graph edges.
   * svtkSplineGraphEdges::CUSTOM uses a svtkCardinalSpline.
   * svtkSplineGraphEdges::BSPLINE uses a b-spline.
   * The default is BSPLINE.
   */
  virtual void SetGraphSplineType(int type, int idx);
  virtual int GetGraphSplineType(int idx);
  //@}

  //@{
  /**
   * The layout strategy for producing spatial regions for the tree.
   */
  virtual void SetAreaLayoutStrategy(svtkAreaLayoutStrategy* strategy);
  virtual svtkAreaLayoutStrategy* GetAreaLayoutStrategy();
  //@}

  //@{
  /**
   * The filter for converting areas to polydata. This may e.g. be
   * svtkTreeMapToPolyData or svtkTreeRingToPolyData.
   * The filter must take a svtkTree as input and produce svtkPolyData.
   */
  virtual void SetAreaToPolyData(svtkPolyDataAlgorithm* areaToPoly);
  svtkGetObjectMacro(AreaToPolyData, svtkPolyDataAlgorithm);
  //@}

  //@{
  /**
   * Whether the area represents radial or rectangular coordinates.
   */
  svtkSetMacro(UseRectangularCoordinates, bool);
  svtkGetMacro(UseRectangularCoordinates, bool);
  svtkBooleanMacro(UseRectangularCoordinates, bool);
  //@}

  //@{
  /**
   * The mapper for rendering labels on areas. This may e.g. be
   * svtkDynamic2DLabelMapper or svtkTreeMapLabelMapper.
   */
  virtual void SetAreaLabelMapper(svtkLabeledDataMapper* mapper);
  svtkGetObjectMacro(AreaLabelMapper, svtkLabeledDataMapper);
  //@}

  /**
   * Apply the theme to this view.
   */
  void ApplyViewTheme(svtkViewTheme* theme) override;

  //@{
  /**
   * Visibility of scalar bar actor for edges.
   */
  virtual void SetEdgeScalarBarVisibility(bool b);
  virtual bool GetEdgeScalarBarVisibility();
  //@}

protected:
  svtkRenderedTreeAreaRepresentation();
  ~svtkRenderedTreeAreaRepresentation() override;

  //@{
  /**
   * Called by the view to add/remove this representation.
   */
  bool AddToView(svtkView* view) override;
  bool RemoveFromView(svtkView* view) override;
  //@}

  svtkSelection* ConvertSelection(svtkView* view, svtkSelection* sel) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void PrepareForRendering(svtkRenderView* view) override;

  bool ValidIndex(int idx);

  void UpdateHoverHighlight(svtkView* view, int x, int y);

  svtkUnicodeString GetHoverTextInternal(svtkSelection* sel) override;

  svtkSmartPointer<svtkWorldPointPicker> Picker;
  svtkSmartPointer<svtkApplyColors> ApplyColors;
  svtkSmartPointer<svtkTreeLevelsFilter> TreeLevels;
  svtkSmartPointer<svtkVertexDegree> VertexDegree;
  svtkSmartPointer<svtkTreeFieldAggregator> TreeAggregation;
  svtkSmartPointer<svtkAreaLayout> AreaLayout;
  svtkSmartPointer<svtkPolyDataMapper> AreaMapper;
  svtkSmartPointer<svtkActor> AreaActor;
  svtkSmartPointer<svtkActor2D> AreaLabelActor;
  svtkSmartPointer<svtkPolyData> HighlightData;
  svtkSmartPointer<svtkPolyDataMapper> HighlightMapper;
  svtkSmartPointer<svtkActor> HighlightActor;
  svtkPolyDataAlgorithm* AreaToPolyData;
  svtkLabeledDataMapper* AreaLabelMapper;
  svtkSmartPointer<svtkScalarBarWidget> EdgeScalarBar;
  svtkSmartPointer<svtkPointSetToLabelHierarchy> AreaLabelHierarchy;
  svtkSmartPointer<svtkPolyData> EmptyPolyData;

  svtkSetStringMacro(AreaSizeArrayNameInternal);
  svtkGetStringMacro(AreaSizeArrayNameInternal);
  char* AreaSizeArrayNameInternal;
  svtkSetStringMacro(AreaColorArrayNameInternal);
  svtkGetStringMacro(AreaColorArrayNameInternal);
  char* AreaColorArrayNameInternal;
  svtkSetStringMacro(AreaLabelArrayNameInternal);
  svtkGetStringMacro(AreaLabelArrayNameInternal);
  char* AreaLabelArrayNameInternal;
  svtkSetStringMacro(AreaLabelPriorityArrayNameInternal);
  svtkGetStringMacro(AreaLabelPriorityArrayNameInternal);
  char* AreaLabelPriorityArrayNameInternal;
  svtkSetStringMacro(GraphEdgeColorArrayNameInternal);
  svtkGetStringMacro(GraphEdgeColorArrayNameInternal);
  char* GraphEdgeColorArrayNameInternal;
  svtkGetStringMacro(AreaHoverTextInternal);
  svtkSetStringMacro(AreaHoverTextInternal);
  char* AreaHoverTextInternal;
  char* AreaHoverArrayName;

  bool UseRectangularCoordinates;

private:
  svtkRenderedTreeAreaRepresentation(const svtkRenderedTreeAreaRepresentation&) = delete;
  void operator=(const svtkRenderedTreeAreaRepresentation&) = delete;

  class Internals;
  Internals* Implementation;
};

#endif
