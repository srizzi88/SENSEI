/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTreeMapHover.h

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
 * @class   svtkInteractorStyleTreeMapHover
 * @brief   An interactor style for a tree map view
 *
 *
 * The svtkInteractorStyleTreeMapHover specifically works with pipelines
 * that create a tree map.  Such pipelines will have a svtkTreeMapLayout
 * filter and a svtkTreeMapToPolyData filter, both of which must be passed
 * to this interactor style for it to function correctly.
 * This interactor style allows only 2D panning and zooming, and additionally
 * provides a balloon containing the name of the vertex hovered over,
 * and allows the user to highlight a vertex by clicking on it.
 */

#ifndef svtkInteractorStyleTreeMapHover_h
#define svtkInteractorStyleTreeMapHover_h

#include "svtkInteractorStyleImage.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkBalloonRepresentation;
class svtkPoints;
class svtkRenderer;
class svtkTree;
class svtkTreeMapLayout;
class svtkTreeMapToPolyData;
class svtkWorldPointPicker;

class SVTKVIEWSINFOVIS_EXPORT svtkInteractorStyleTreeMapHover : public svtkInteractorStyleImage
{
public:
  static svtkInteractorStyleTreeMapHover* New();
  svtkTypeMacro(svtkInteractorStyleTreeMapHover, svtkInteractorStyleImage);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Must be set to the svtkTreeMapLayout used to compute the bounds of each vertex
   * for the tree map.
   */
  void SetLayout(svtkTreeMapLayout* layout);
  svtkGetObjectMacro(Layout, svtkTreeMapLayout);
  //@}

  //@{
  /**
   * Must be set to the svtkTreeMapToPolyData used to convert the tree map
   * into polydata.
   */
  void SetTreeMapToPolyData(svtkTreeMapToPolyData* filter);
  svtkGetObjectMacro(TreeMapToPolyData, svtkTreeMapToPolyData);
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
   * Overridden from svtkInteractorStyleImage to provide the desired
   * interaction behavior.
   */
  void OnMouseMove() override;
  void OnLeftButtonUp() override;
  //@}

  //@{
  /**
   * Highlights a specific vertex.
   */
  void HighLightItem(svtkIdType id);
  void HighLightCurrentSelectedItem();
  //@}

  void SetInteractor(svtkRenderWindowInteractor* rwi) override;

  /**
   * Set the color used to highlight the hovered vertex.
   */
  void SetHighLightColor(double r, double g, double b);

  /**
   * Set the color used to highlight the selected vertex.
   */
  void SetSelectionLightColor(double r, double g, double b);

  //@{
  /**
   * The width of the line around the hovered vertex.
   */
  void SetHighLightWidth(double lw);
  double GetHighLightWidth();
  //@}

  //@{
  /**
   * The width of the line around the selected vertex.
   */
  void SetSelectionWidth(double lw);
  double GetSelectionWidth();
  //@}

protected:
  svtkInteractorStyleTreeMapHover();
  ~svtkInteractorStyleTreeMapHover() override;

private:
  svtkInteractorStyleTreeMapHover(const svtkInteractorStyleTreeMapHover&) = delete;
  void operator=(const svtkInteractorStyleTreeMapHover&) = delete;

  // These methods are used internally
  svtkIdType GetTreeMapIdAtPos(int x, int y);
  void GetBoundingBoxForTreeMapItem(svtkIdType id, float* binfo);

  svtkWorldPointPicker* Picker;
  svtkBalloonRepresentation* Balloon;
  svtkActor* HighlightActor;
  svtkActor* SelectionActor;
  svtkPoints* HighlightPoints;
  svtkPoints* SelectionPoints;
  svtkTreeMapLayout* Layout;
  svtkTreeMapToPolyData* TreeMapToPolyData;
  char* LabelField;
  svtkIdType CurrentSelectedId;
};

#endif
