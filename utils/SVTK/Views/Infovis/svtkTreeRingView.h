/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeRingView.h

  -------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTreeRingView
 * @brief   Displays a tree in concentric rings.
 *
 *
 * Accepts a graph and a hierarchy - currently
 * a tree - and provides a hierarchy-aware display.  Currently, this means
 * displaying the hierarchy using a tree ring layout, then rendering the graph
 * vertices as leaves of the tree with curved graph edges between leaves.
 *
 * .SEE ALSO
 * svtkGraphLayoutView
 *
 * @par Thanks:
 * Thanks to Jason Shepherd for implementing this class
 */

#ifndef svtkTreeRingView_h
#define svtkTreeRingView_h

#include "svtkTreeAreaView.h"
#include "svtkViewsInfovisModule.h" // For export macro

class SVTKVIEWSINFOVIS_EXPORT svtkTreeRingView : public svtkTreeAreaView
{
public:
  static svtkTreeRingView* New();
  svtkTypeMacro(svtkTreeRingView, svtkTreeAreaView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the root angles for laying out the hierarchy.
   */
  void SetRootAngles(double start, double end);

  //@{
  /**
   * Sets whether the root is at the center or around the outside.
   */
  virtual void SetRootAtCenter(bool value);
  virtual bool GetRootAtCenter();
  svtkBooleanMacro(RootAtCenter, bool);
  //@}

  //@{
  /**
   * Set the thickness of each layer.
   */
  virtual void SetLayerThickness(double thickness);
  virtual double GetLayerThickness();
  //@}

  //@{
  /**
   * Set the interior radius of the tree
   * (i.e. the size of the "hole" in the center).
   */
  virtual void SetInteriorRadius(double thickness);
  virtual double GetInteriorRadius();
  //@}

  //@{
  /**
   * Set the log spacing factor for the invisible interior tree
   * used for routing edges of the overlaid graph.
   */
  virtual void SetInteriorLogSpacingValue(double thickness);
  virtual double GetInteriorLogSpacingValue();
  //@}

protected:
  svtkTreeRingView();
  ~svtkTreeRingView() override;

private:
  svtkTreeRingView(const svtkTreeRingView&) = delete;
  void operator=(const svtkTreeRingView&) = delete;
};

#endif
