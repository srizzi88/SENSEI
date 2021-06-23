/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedSurfaceRepresentation.h

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
 * @class   svtkRenderedSurfaceRepresentation
 * @brief   Displays a geometric dataset as a surface.
 *
 *
 * svtkRenderedSurfaceRepresentation is used to show a geometric dataset in a view.
 * The representation uses a svtkGeometryFilter to convert the dataset to
 * polygonal data (e.g. volumetric data is converted to its external surface).
 * The representation may then be added to svtkRenderView.
 */

#ifndef svtkRenderedSurfaceRepresentation_h
#define svtkRenderedSurfaceRepresentation_h

#include "svtkRenderedRepresentation.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkActor;
class svtkAlgorithmOutput;
class svtkApplyColors;
class svtkDataObject;
class svtkGeometryFilter;
class svtkPolyDataMapper;
class svtkRenderView;
class svtkScalarsToColors;
class svtkSelection;
class svtkTransformFilter;
class svtkView;

class SVTKVIEWSINFOVIS_EXPORT svtkRenderedSurfaceRepresentation : public svtkRenderedRepresentation
{
public:
  static svtkRenderedSurfaceRepresentation* New();
  svtkTypeMacro(svtkRenderedSurfaceRepresentation, svtkRenderedRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Sets the color array name
   */
  virtual void SetCellColorArrayName(const char* arrayName);
  virtual const char* GetCellColorArrayName() { return this->GetCellColorArrayNameInternal(); }

  /**
   * Apply a theme to this representation.
   */
  void ApplyViewTheme(svtkViewTheme* theme) override;

protected:
  svtkRenderedSurfaceRepresentation();
  ~svtkRenderedSurfaceRepresentation() override;

  /**
   * Sets the input pipeline connection to this representation.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Performs per-render operations.
   */
  void PrepareForRendering(svtkRenderView* view) override;

  /**
   * Adds the representation to the view.  This is called from
   * svtkView::AddRepresentation().
   */
  bool AddToView(svtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * svtkView::RemoveRepresentation().
   */
  bool RemoveFromView(svtkView* view) override;

  /**
   * Convert the selection to a type appropriate for sharing with other
   * representations through svtkAnnotationLink.
   * If the selection cannot be applied to this representation, returns nullptr.
   */
  svtkSelection* ConvertSelection(svtkView* view, svtkSelection* selection) override;

  //@{
  /**
   * Internal pipeline objects.
   */
  svtkTransformFilter* TransformFilter;
  svtkApplyColors* ApplyColors;
  svtkGeometryFilter* GeometryFilter;
  svtkPolyDataMapper* Mapper;
  svtkActor* Actor;
  //@}

  svtkGetStringMacro(CellColorArrayNameInternal);
  svtkSetStringMacro(CellColorArrayNameInternal);
  char* CellColorArrayNameInternal;

private:
  svtkRenderedSurfaceRepresentation(const svtkRenderedSurfaceRepresentation&) = delete;
  void operator=(const svtkRenderedSurfaceRepresentation&) = delete;
};

#endif
