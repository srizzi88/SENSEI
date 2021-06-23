/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedHierarchyRepresentation.h

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
 * @class   svtkRenderedHierarchyRepresentation
 *
 *
 */

#ifndef svtkRenderedHierarchyRepresentation_h
#define svtkRenderedHierarchyRepresentation_h

#include "svtkRenderedGraphRepresentation.h"
#include "svtkViewsInfovisModule.h" // For export macro

class SVTKVIEWSINFOVIS_EXPORT svtkRenderedHierarchyRepresentation
  : public svtkRenderedGraphRepresentation
{
public:
  static svtkRenderedHierarchyRepresentation* New();
  svtkTypeMacro(svtkRenderedHierarchyRepresentation, svtkRenderedGraphRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**

   */
  virtual void SetGraphEdgeLabelArrayName(const char* name)
  {
    this->SetGraphEdgeLabelArrayName(name, 0);
  }
  virtual void SetGraphEdgeLabelArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeLabelArrayName() { return this->GetGraphEdgeLabelArrayName(0); }
  virtual const char* GetGraphEdgeLabelArrayName(int idx);
  //@}

  virtual void SetGraphEdgeLabelVisibility(bool vis) { this->SetGraphEdgeLabelVisibility(vis, 0); }
  virtual void SetGraphEdgeLabelVisibility(bool vis, int idx);
  virtual bool GetGraphEdgeLabelVisibility() { return this->GetGraphEdgeLabelVisibility(0); }
  virtual bool GetGraphEdgeLabelVisibility(int idx);
  svtkBooleanMacro(GraphEdgeLabelVisibility, bool);

  virtual void SetGraphEdgeColorArrayName(const char* name)
  {
    this->SetGraphEdgeColorArrayName(name, 0);
  }
  virtual void SetGraphEdgeColorArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeColorArrayName() { return this->GetGraphEdgeColorArrayName(0); }
  virtual const char* GetGraphEdgeColorArrayName(int idx);

  virtual void SetColorGraphEdgesByArray(bool vis) { this->SetColorGraphEdgesByArray(vis, 0); }
  virtual void SetColorGraphEdgesByArray(bool vis, int idx);
  virtual bool GetColorGraphEdgesByArray() { return this->GetColorGraphEdgesByArray(0); }
  virtual bool GetColorGraphEdgesByArray(int idx);
  svtkBooleanMacro(ColorGraphEdgesByArray, bool);

  virtual void SetGraphEdgeColorToSplineFraction()
  {
    this->SetGraphEdgeColorArrayName("fraction", 0);
  }
  virtual void SetGraphEdgeColorToSplineFraction(int idx)
  {
    this->SetGraphEdgeColorArrayName("fraction", idx);
  }

  virtual void SetGraphVisibility(bool vis) { this->SetGraphVisibility(vis, 0); }
  virtual void SetGraphVisibility(bool vis, int idx);
  virtual bool GetGraphVisibility() { return this->GetGraphVisibility(0); }
  virtual bool GetGraphVisibility(int idx);
  svtkBooleanMacro(GraphVisibility, bool);

  virtual void SetBundlingStrength(double strength) { this->SetBundlingStrength(strength, 0); }
  virtual void SetBundlingStrength(double strength, int idx);
  virtual double GetBundlingStrength() { return this->GetBundlingStrength(0); }
  virtual double GetBundlingStrength(int idx);

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

  virtual void SetGraphEdgeLabelFontSize(int size) { this->SetGraphEdgeLabelFontSize(size, 0); }
  virtual void SetGraphEdgeLabelFontSize(int size, int idx);
  virtual int GetGraphEdgeLabelFontSize() { return this->GetGraphEdgeLabelFontSize(0); }
  virtual int GetGraphEdgeLabelFontSize(int idx);

protected:
  svtkRenderedHierarchyRepresentation();
  ~svtkRenderedHierarchyRepresentation() override;

  //@{
  /**
   * Called by the view to add/remove this representation.
   */
  bool AddToView(svtkView* view) override;
  bool RemoveFromView(svtkView* view) override;
  //@}

  /**
   * Whether idx is a valid graph index.
   */
  bool ValidIndex(int idx);

  svtkSelection* ConvertSelection(svtkView* view, svtkSelection* sel) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Sets up the input connections for this representation.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  void ApplyViewTheme(svtkViewTheme* theme) override;

  class Internals;
  Internals* Implementation;

private:
  svtkRenderedHierarchyRepresentation(const svtkRenderedHierarchyRepresentation&) = delete;
  void operator=(const svtkRenderedHierarchyRepresentation&) = delete;
};

#endif
