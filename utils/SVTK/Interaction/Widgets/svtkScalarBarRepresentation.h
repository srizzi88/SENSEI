/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   svtkScalarBarRepresentation
 * @brief   represent scalar bar for svtkScalarBarWidget
 *
 *
 *
 * This class represents a scalar bar for a svtkScalarBarWidget.  This class
 * provides support for interactively placing a scalar bar on the 2D overlay
 * plane.  The scalar bar is defined by an instance of svtkScalarBarActor.
 *
 * One specialty of this class is that if the scalar bar is moved near enough
 * to an edge, it's orientation is flipped to match that edge.
 *
 * @sa
 * svtkScalarBarWidget svtkWidgetRepresentation svtkScalarBarActor
 *
 */

#ifndef svtkScalarBarRepresentation_h
#define svtkScalarBarRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkScalarBarActor;

class SVTKINTERACTIONWIDGETS_EXPORT svtkScalarBarRepresentation : public svtkBorderRepresentation
{
public:
  svtkTypeMacro(svtkScalarBarRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkScalarBarRepresentation* New();

  //@{
  /**
   * The prop that is placed in the renderer.
   */
  svtkGetObjectMacro(ScalarBarActor, svtkScalarBarActor);
  virtual void SetScalarBarActor(svtkScalarBarActor*);
  //@}

  //@{
  /**
   * Satisfy the superclass' API.
   */
  void BuildRepresentation() override;
  void WidgetInteraction(double eventPos[2]) override;
  void GetSize(double size[2]) override
  {
    size[0] = 2.0;
    size[1] = 2.0;
  }
  //@}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  svtkTypeBool GetVisibility() override;
  void SetVisibility(svtkTypeBool) override;
  void GetActors2D(svtkPropCollection* collection) override;
  void ReleaseGraphicsResources(svtkWindow* window) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * If true, the orientation will be updated based on the widget's position.
   * Default is true.
   */
  svtkSetMacro(AutoOrient, bool);
  svtkGetMacro(AutoOrient, bool);
  //@}

  //@{
  /**
   * Get/Set the orientation.
   */
  void SetOrientation(int orient);
  int GetOrientation();
  //@}

protected:
  svtkScalarBarRepresentation();
  ~svtkScalarBarRepresentation() override;

  /**
   * Change horizontal <--> vertical orientation, rotate the corners of the
   * bar to preserve size, and swap the resize handle locations.
   */
  void SwapOrientation();

  svtkScalarBarActor* ScalarBarActor;
  bool AutoOrient;

private:
  svtkScalarBarRepresentation(const svtkScalarBarRepresentation&) = delete;
  void operator=(const svtkScalarBarRepresentation&) = delete;
};

#endif // svtkScalarBarRepresentation_h
