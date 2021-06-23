/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalGraphPipeline.h

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
 * @class   svtkHierarchicalGraphPipeline
 * @brief   helper class for rendering graphs superimposed on a tree.
 *
 *
 * svtkHierarchicalGraphPipeline renders bundled edges that are meant to be
 * viewed as an overlay on a tree. This class is not for general use, but
 * is used in the internals of svtkRenderedHierarchyRepresentation and
 * svtkRenderedTreeAreaRepresentation.
 */

#ifndef svtkHierarchicalGraphPipeline_h
#define svtkHierarchicalGraphPipeline_h

#include "svtkObject.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkActor;
class svtkActor2D;
class svtkAlgorithmOutput;
class svtkApplyColors;
class svtkDataRepresentation;
class svtkDynamic2DLabelMapper;
class svtkEdgeCenters;
class svtkGraphHierarchicalBundleEdges;
class svtkGraphToPolyData;
class svtkPolyDataMapper;
class svtkRenderView;
class svtkSplineGraphEdges;
class svtkSelection;
class svtkTextProperty;
class svtkViewTheme;

class SVTKVIEWSINFOVIS_EXPORT svtkHierarchicalGraphPipeline : public svtkObject
{
public:
  static svtkHierarchicalGraphPipeline* New();
  svtkTypeMacro(svtkHierarchicalGraphPipeline, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The actor associated with the hierarchical graph.
   */
  svtkGetObjectMacro(Actor, svtkActor);
  //@}

  //@{
  /**
   * The actor associated with the hierarchical graph.
   */
  svtkGetObjectMacro(LabelActor, svtkActor2D);
  //@}

  //@{
  /**
   * The bundling strength for the bundled edges.
   */
  virtual void SetBundlingStrength(double strength);
  virtual double GetBundlingStrength();
  //@}

  //@{
  /**
   * The edge label array name.
   */
  virtual void SetLabelArrayName(const char* name);
  virtual const char* GetLabelArrayName();
  //@}

  //@{
  /**
   * The edge label visibility.
   */
  virtual void SetLabelVisibility(bool vis);
  virtual bool GetLabelVisibility();
  svtkBooleanMacro(LabelVisibility, bool);
  //@}

  //@{
  /**
   * The edge label text property.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* prop);
  virtual svtkTextProperty* GetLabelTextProperty();
  //@}

  //@{
  /**
   * The edge color array.
   */
  virtual void SetColorArrayName(const char* name);
  virtual const char* GetColorArrayName();
  //@}

  //@{
  /**
   * Whether to color the edges by an array.
   */
  virtual void SetColorEdgesByArray(bool vis);
  virtual bool GetColorEdgesByArray();
  svtkBooleanMacro(ColorEdgesByArray, bool);
  //@}

  //@{
  /**
   * The visibility of this graph.
   */
  virtual void SetVisibility(bool vis);
  virtual bool GetVisibility();
  svtkBooleanMacro(Visibility, bool);
  //@}

  /**
   * Returns a new selection relevant to this graph based on an input
   * selection and the view that this graph is contained in.
   */
  virtual svtkSelection* ConvertSelection(svtkDataRepresentation* rep, svtkSelection* sel);

  /**
   * Sets the input connections for this graph.
   * graphConn is the input graph connection.
   * treeConn is the input tree connection.
   * annConn is the annotation link connection.
   */
  virtual void PrepareInputConnections(
    svtkAlgorithmOutput* graphConn, svtkAlgorithmOutput* treeConn, svtkAlgorithmOutput* annConn);

  /**
   * Applies the view theme to this graph.
   */
  virtual void ApplyViewTheme(svtkViewTheme* theme);

  //@{
  /**
   * The array to use while hovering over an edge.
   */
  svtkSetStringMacro(HoverArrayName);
  svtkGetStringMacro(HoverArrayName);
  //@}

  //@{
  /**
   * The spline mode to use in svtkSplineGraphEdges.
   * svtkSplineGraphEdges::CUSTOM uses a svtkCardinalSpline.
   * svtkSplineGraphEdges::BSPLINE uses a b-spline.
   * The default is BSPLINE.
   */
  virtual void SetSplineType(int type);
  virtual int GetSplineType();
  //@}

  /**
   * Register progress with a view.
   */
  void RegisterProgress(svtkRenderView* view);

protected:
  svtkHierarchicalGraphPipeline();
  ~svtkHierarchicalGraphPipeline() override;

  svtkApplyColors* ApplyColors;
  svtkGraphHierarchicalBundleEdges* Bundle;
  svtkGraphToPolyData* GraphToPoly;
  svtkSplineGraphEdges* Spline;
  svtkPolyDataMapper* Mapper;
  svtkActor* Actor;
  svtkTextProperty* TextProperty;
  svtkEdgeCenters* EdgeCenters;
  svtkDynamic2DLabelMapper* LabelMapper;
  svtkActor2D* LabelActor;

  char* HoverArrayName;

  svtkSetStringMacro(ColorArrayNameInternal);
  svtkGetStringMacro(ColorArrayNameInternal);
  char* ColorArrayNameInternal;

  svtkSetStringMacro(LabelArrayNameInternal);
  svtkGetStringMacro(LabelArrayNameInternal);
  char* LabelArrayNameInternal;

private:
  svtkHierarchicalGraphPipeline(const svtkHierarchicalGraphPipeline&) = delete;
  void operator=(const svtkHierarchicalGraphPipeline&) = delete;
};

#endif
