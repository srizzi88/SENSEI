/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIcicleView.h

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
 * @class   svtkIcicleView
 * @brief   Displays a tree in a stacked "icicle" view
 *
 *
 * svtkIcicleView shows a svtkTree in horizontal layers
 * where each vertex in the tree is represented by a bar.
 * Child sectors are below (or above) parent sectors, and may be
 * colored and sized by various parameters.
 */

#ifndef svtkIcicleView_h
#define svtkIcicleView_h

#include "svtkTreeAreaView.h"
#include "svtkViewsInfovisModule.h" // For export macro

class SVTKVIEWSINFOVIS_EXPORT svtkIcicleView : public svtkTreeAreaView
{
public:
  static svtkIcicleView* New();
  svtkTypeMacro(svtkIcicleView, svtkTreeAreaView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sets whether the stacks go from top to bottom or bottom to top.
   */
  virtual void SetTopToBottom(bool value);
  virtual bool GetTopToBottom();
  svtkBooleanMacro(TopToBottom, bool);
  //@}

  //@{
  /**
   * Set the width of the root node
   */
  virtual void SetRootWidth(double width);
  virtual double GetRootWidth();
  //@}

  //@{
  /**
   * Set the thickness of each layer
   */
  virtual void SetLayerThickness(double thickness);
  virtual double GetLayerThickness();
  //@}

  //@{
  /**
   * Turn on/off gradient coloring.
   */
  virtual void SetUseGradientColoring(bool value);
  virtual bool GetUseGradientColoring();
  svtkBooleanMacro(UseGradientColoring, bool);
  //@}

protected:
  svtkIcicleView();
  ~svtkIcicleView() override;

private:
  svtkIcicleView(const svtkIcicleView&) = delete;
  void operator=(const svtkIcicleView&) = delete;
};

#endif
