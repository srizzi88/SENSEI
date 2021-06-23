/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeMapView.h

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
 * @class   svtkTreeMapView
 * @brief   Displays a tree as a tree map.
 *
 *
 * svtkTreeMapView shows a svtkTree in a tree map, where each vertex in the
 * tree is represented by a box.  Child boxes are contained within the
 * parent box, and may be colored and sized by various parameters.
 */

#ifndef svtkTreeMapView_h
#define svtkTreeMapView_h

#include "svtkTreeAreaView.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkBoxLayoutStrategy;
class svtkSliceAndDiceLayoutStrategy;
class svtkSquarifyLayoutStrategy;

class SVTKVIEWSINFOVIS_EXPORT svtkTreeMapView : public svtkTreeAreaView
{
public:
  static svtkTreeMapView* New();
  svtkTypeMacro(svtkTreeMapView, svtkTreeAreaView);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sets the treemap layout strategy
   */
  void SetLayoutStrategy(svtkAreaLayoutStrategy* s) override;
  virtual void SetLayoutStrategy(const char* name);
  virtual void SetLayoutStrategyToBox();
  virtual void SetLayoutStrategyToSliceAndDice();
  virtual void SetLayoutStrategyToSquarify();
  //@}

  //@{
  /**
   * The sizes of the fonts used for labeling.
   */
  virtual void SetFontSizeRange(const int maxSize, const int minSize, const int delta = 4);
  virtual void GetFontSizeRange(int range[3]);
  //@}

protected:
  svtkTreeMapView();
  ~svtkTreeMapView() override;

  svtkSmartPointer<svtkBoxLayoutStrategy> BoxLayout;
  svtkSmartPointer<svtkSliceAndDiceLayoutStrategy> SliceAndDiceLayout;
  svtkSmartPointer<svtkSquarifyLayoutStrategy> SquarifyLayout;

private:
  svtkTreeMapView(const svtkTreeMapView&) = delete;
  void operator=(const svtkTreeMapView&) = delete;
};

#endif
