/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkParallelCoordinatesView.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkParallelCoordinatesView
 * @brief   view to be used with svtkParallelCoordinatesRepresentation
 *
 *
 *
 * This class manages interaction with the svtkParallelCoordinatesRepresentation.  There are
 * two inspection modes: axis manipulation and line selection.  In axis manipulation mode,
 * PC axes can be dragged and reordered with the LMB, axis ranges can be increased/decreased
 * by dragging up/down with the LMB, and RMB controls zoom and pan.
 *
 * In line selection mode, there are three subclasses of selections: lasso, angle, and
 * function selection.  Lasso selection lets the user brush a line and select all PC lines
 * that pass nearby.  Angle selection lets the user draw a representative line between axes
 * and select all lines that have similar orientation.  Function selection lets the user
 * draw two representative lines between a pair of axes and select all lines that match
 * the linear interpolation of those lines.
 *
 * There are several self-explanatory operators for combining selections: ADD, SUBTRACT
 * REPLACE, and INTERSECT.
 */

#ifndef svtkParallelCoordinatesView_h
#define svtkParallelCoordinatesView_h

#include "svtkRenderView.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkActor2D;
class svtkOutlineSource;
class svtkParallelCoordinatesRepresentation;
class svtkPolyData;
class svtkPolyDataMapper2D;

class SVTKVIEWSINFOVIS_EXPORT svtkParallelCoordinatesView : public svtkRenderView
{
public:
  svtkTypeMacro(svtkParallelCoordinatesView, svtkRenderView);
  static svtkParallelCoordinatesView* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    SVTK_BRUSH_LASSO = 0,
    SVTK_BRUSH_ANGLE,
    SVTK_BRUSH_FUNCTION,
    SVTK_BRUSH_AXISTHRESHOLD,
    SVTK_BRUSH_MODECOUNT
  };
  enum
  {
    SVTK_BRUSHOPERATOR_ADD = 0,
    SVTK_BRUSHOPERATOR_SUBTRACT,
    SVTK_BRUSHOPERATOR_INTERSECT,
    SVTK_BRUSHOPERATOR_REPLACE,
    SVTK_BRUSHOPERATOR_MODECOUNT
  };
  enum
  {
    SVTK_INSPECT_MANIPULATE_AXES = 0,
    SVTK_INSPECT_SELECT_DATA,
    SVTK_INSPECT_MODECOUNT
  };

  void SetBrushMode(int);
  void SetBrushModeToLasso() { this->SetBrushMode(SVTK_BRUSH_LASSO); }
  void SetBrushModeToAngle() { this->SetBrushMode(SVTK_BRUSH_ANGLE); }
  void SetBrushModeToFunction() { this->SetBrushMode(SVTK_BRUSH_FUNCTION); }
  void SetBrushModeToAxisThreshold() { this->SetBrushMode(SVTK_BRUSH_AXISTHRESHOLD); }
  svtkGetMacro(BrushMode, int);

  void SetBrushOperator(int);
  void SetBrushOperatorToAdd() { this->SetBrushOperator(SVTK_BRUSHOPERATOR_ADD); }
  void SetBrushOperatorToSubtract() { this->SetBrushOperator(SVTK_BRUSHOPERATOR_SUBTRACT); }
  void SetBrushOperatorToIntersect() { this->SetBrushOperator(SVTK_BRUSHOPERATOR_INTERSECT); }
  void SetBrushOperatorToReplace() { this->SetBrushOperator(SVTK_BRUSHOPERATOR_REPLACE); }
  svtkGetMacro(BrushOperator, int);

  void SetInspectMode(int);
  void SetInspectModeToManipulateAxes() { this->SetInspectMode(SVTK_INSPECT_MANIPULATE_AXES); }
  void SetInpsectModeToSelectData() { this->SetInspectMode(SVTK_INSPECT_SELECT_DATA); }
  svtkGetMacro(InspectMode, int);

  void SetMaximumNumberOfBrushPoints(int);
  svtkGetMacro(MaximumNumberOfBrushPoints, int);

  svtkSetMacro(CurrentBrushClass, int);
  svtkGetMacro(CurrentBrushClass, int);

  void ApplyViewTheme(svtkViewTheme* theme) override;

protected:
  svtkParallelCoordinatesView();
  ~svtkParallelCoordinatesView() override;

  int SelectedAxisPosition;

  enum
  {
    SVTK_HIGHLIGHT_CENTER = 0,
    SVTK_HIGHLIGHT_MIN,
    SVTK_HIGHLIGHT_MAX
  };
  svtkSmartPointer<svtkOutlineSource> HighlightSource;
  svtkSmartPointer<svtkPolyDataMapper2D> HighlightMapper;
  svtkSmartPointer<svtkActor2D> HighlightActor;

  int InspectMode;
  int BrushMode;
  int BrushOperator;
  int MaximumNumberOfBrushPoints;
  int NumberOfBrushPoints;
  int CurrentBrushClass;

  svtkSmartPointer<svtkPolyData> BrushData;
  svtkSmartPointer<svtkPolyDataMapper2D> BrushMapper;
  svtkSmartPointer<svtkActor2D> BrushActor;

  int FirstFunctionBrushLineDrawn;
  int AxisHighlightPosition;

  svtkTimeStamp WorldBuildTime;
  bool RebuildNeeded;

  void ProcessEvents(svtkObject* caller, unsigned long event, void* callData) override;
  svtkDataRepresentation* CreateDefaultRepresentation(svtkAlgorithmOutput* conn) override;

  void PrepareForRendering() override;

  //@{
  /**
   * Handle axis manipulation
   */
  void Hover(unsigned long event);
  void ManipulateAxes(unsigned long event);
  void SelectData(unsigned long event);
  void Zoom(unsigned long event);
  void Pan(unsigned long event);
  //@}

  /**
   * Set/Get the position of axis highlights
   */
  int SetAxisHighlightPosition(svtkParallelCoordinatesRepresentation* rep, int position);

  /**
   * Set the highlight position using normalized viewport coordinates
   */
  int SetAxisHighlightPosition(svtkParallelCoordinatesRepresentation* rep, double position);

  int AddLassoBrushPoint(double* p);
  int SetBrushLine(int line, double* p1, double* p2);
  void GetBrushLine(int line, svtkIdType& npts, svtkIdType const*& ptids);
  int SetAngleBrushLine(double* p1, double* p2);
  int SetFunctionBrushLine1(double* p1, double* p2);
  int SetFunctionBrushLine2(double* p1, double* p2);
  void ClearBrushPoints();

private:
  svtkParallelCoordinatesView(const svtkParallelCoordinatesView&) = delete;
  void operator=(const svtkParallelCoordinatesView&) = delete;
};

#endif
