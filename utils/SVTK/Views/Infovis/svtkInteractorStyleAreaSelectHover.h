/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleAreaSelectHover.h

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
 * @class   svtkInteractorStyleAreaSelectHover
 * @brief   An interactor style for an area tree view
 *
 *
 * The svtkInteractorStyleAreaSelectHover specifically works with pipelines
 * that create a hierarchical tree.  Such pipelines will have a svtkAreaLayout
 * filter which must be passed to this interactor style for it to function
 * correctly.
 * This interactor style allows only 2D panning and zooming,
 * rubber band selection and provides a balloon containing the name of the
 * vertex hovered over.
 */

#ifndef svtkInteractorStyleAreaSelectHover_h
#define svtkInteractorStyleAreaSelectHover_h

#include "svtkInteractorStyleRubberBand2D.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkAreaLayout;
class svtkBalloonRepresentation;
class svtkPoints;
class svtkRenderer;
class svtkTree;
class svtkWorldPointPicker;
class svtkPolyData;

class SVTKVIEWSINFOVIS_EXPORT svtkInteractorStyleAreaSelectHover
  : public svtkInteractorStyleRubberBand2D
{
public:
  static svtkInteractorStyleAreaSelectHover* New();
  svtkTypeMacro(svtkInteractorStyleAreaSelectHover, svtkInteractorStyleRubberBand2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Must be set to the svtkAreaLayout used to compute the bounds of
   * each vertex.
   */
  void SetLayout(svtkAreaLayout* layout);
  svtkGetObjectMacro(Layout, svtkAreaLayout);
  //@}

  //@{
  /**
   * The name of the field to use when displaying text in the hover balloon.
   */
  svtkSetStringMacro(LabelField);
  svtkGetStringMacro(LabelField);
  //@}

  //@{
  /**
   * Determine whether or not to use rectangular coordinates instead of
   * polar coordinates.
   */
  svtkSetMacro(UseRectangularCoordinates, bool);
  svtkGetMacro(UseRectangularCoordinates, bool);
  svtkBooleanMacro(UseRectangularCoordinates, bool);
  //@}

  /**
   * Overridden from svtkInteractorStyleImage to provide the desired
   * interaction behavior.
   */
  void OnMouseMove() override;

  /**
   * Set the interactor that this interactor style works with.
   */
  void SetInteractor(svtkRenderWindowInteractor* rwi) override;

  /**
   * Set the color used to highlight the hovered vertex.
   */
  void SetHighLightColor(double r, double g, double b);

  //@{
  /**
   * The width of the line around the hovered vertex.
   */
  void SetHighLightWidth(double lw);
  double GetHighLightWidth();
  //@}

  /**
   * Obtain the tree vertex id at the position specified.
   */
  svtkIdType GetIdAtPos(int x, int y);

protected:
  svtkInteractorStyleAreaSelectHover();
  ~svtkInteractorStyleAreaSelectHover() override;

private:
  svtkInteractorStyleAreaSelectHover(const svtkInteractorStyleAreaSelectHover&) = delete;
  void operator=(const svtkInteractorStyleAreaSelectHover&) = delete;

  // These methods are used internally
  void GetBoundingAreaForItem(svtkIdType id, float* sinfo);

  svtkWorldPointPicker* Picker;
  svtkBalloonRepresentation* Balloon;
  svtkPolyData* HighlightData;
  svtkActor* HighlightActor;
  svtkAreaLayout* Layout;
  char* LabelField;
  bool UseRectangularCoordinates;
};

#endif
