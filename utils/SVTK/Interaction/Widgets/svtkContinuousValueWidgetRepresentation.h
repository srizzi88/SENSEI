/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContinuousValueWidgetRepresentation.h

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
 * @class   svtkContinuousValueWidgetRepresentation
 * @brief   provide the representation for a continuous value
 *
 * This class is used mainly as a superclass for continuous value widgets
 *
 */

#ifndef svtkContinuousValueWidgetRepresentation_h
#define svtkContinuousValueWidgetRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class SVTKINTERACTIONWIDGETS_EXPORT svtkContinuousValueWidgetRepresentation
  : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkContinuousValueWidgetRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Methods to interface with the svtkSliderWidget. The PlaceWidget() method
   * assumes that the parameter bounds[6] specifies the location in display
   * space where the widget should be placed.
   */
  void PlaceWidget(double bounds[6]) override;
  void BuildRepresentation() override {}
  void StartWidgetInteraction(double eventPos[2]) override = 0;
  void WidgetInteraction(double eventPos[2]) override = 0;
  //  virtual void Highlight(int);
  //@}

  // Enums are used to describe what is selected
  enum _InteractionState
  {
    Outside = 0,
    Inside,
    Adjusting
  };

  // Set/Get the value
  virtual void SetValue(double value);
  virtual double GetValue() { return this->Value; }

protected:
  svtkContinuousValueWidgetRepresentation();
  ~svtkContinuousValueWidgetRepresentation() override;

  double Value;

private:
  svtkContinuousValueWidgetRepresentation(const svtkContinuousValueWidgetRepresentation&) = delete;
  void operator=(const svtkContinuousValueWidgetRepresentation&) = delete;
};

#endif
