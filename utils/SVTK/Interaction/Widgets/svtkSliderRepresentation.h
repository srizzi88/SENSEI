/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliderRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSliderRepresentation
 * @brief   abstract class defines the representation for a svtkSliderWidget
 *
 * This abstract class is used to specify how the svtkSliderWidget should
 * interact with representations of the svtkSliderWidget. This class may be
 * subclassed so that alternative representations can be created. The class
 * defines an API, and a default implementation, that the svtkSliderWidget
 * interacts with to render itself in the scene.
 *
 * @sa
 * svtkSliderWidget
 */

#ifndef svtkSliderRepresentation_h
#define svtkSliderRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class SVTKINTERACTIONWIDGETS_EXPORT svtkSliderRepresentation : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard methods for the class.
   */
  svtkTypeMacro(svtkSliderRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the current value for the widget. The value should lie between
   * the minimum and maximum values.
   */
  void SetValue(double value);
  svtkGetMacro(Value, double);
  //@}

  //@{
  /**
   * Set the current minimum value that the slider can take. Setting the
   * minimum value greater than the maximum value will cause the maximum
   * value to grow to (minimum value + 1).
   */
  void SetMinimumValue(double value);
  svtkGetMacro(MinimumValue, double);
  //@}

  //@{
  /**
   * Set the current maximum value that the slider can take. Setting the
   * maximum value less than the minimum value will cause the minimum
   * value to change to (maximum value - 1).
   */
  void SetMaximumValue(double value);
  svtkGetMacro(MaximumValue, double);
  //@}

  //@{
  /**
   * Specify the length of the slider shape (in normalized display coordinates
   * [0.01,0.5]). The slider length by default is 0.05.
   */
  svtkSetClampMacro(SliderLength, double, 0.01, 0.5);
  svtkGetMacro(SliderLength, double);
  //@}

  //@{
  /**
   * Set the width of the slider in the directions orthogonal to the
   * slider axis. Using this it is possible to create ellipsoidal and hockey
   * puck sliders (in some subclasses). By default the width is 0.05.
   */
  svtkSetClampMacro(SliderWidth, double, 0.0, 1.0);
  svtkGetMacro(SliderWidth, double);
  //@}

  //@{
  /**
   * Set the width of the tube (in normalized display coordinates) on which
   * the slider moves. By default the width is 0.05.
   */
  svtkSetClampMacro(TubeWidth, double, 0.0, 1.0);
  svtkGetMacro(TubeWidth, double);
  //@}

  //@{
  /**
   * Specify the length of each end cap (in normalized coordinates
   * [0.0,0.25]). By default the length is 0.025. If the end cap length
   * is set to 0.0, then the end cap will not display at all.
   */
  svtkSetClampMacro(EndCapLength, double, 0.0, 0.25);
  svtkGetMacro(EndCapLength, double);
  //@}

  //@{
  /**
   * Specify the width of each end cap (in normalized coordinates
   * [0.0,0.25]). By default the width is twice the tube width.
   */
  svtkSetClampMacro(EndCapWidth, double, 0.0, 0.25);
  svtkGetMacro(EndCapWidth, double);
  //@}

  /**
   * Specify the label text for this widget. If the value is not set, or set
   * to the empty string "", then the label text is not displayed.
   */
  virtual void SetTitleText(const char*) {}
  virtual const char* GetTitleText() { return nullptr; }

  //@{
  /**
   * Set/Get the format with which to print the slider value.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Specify the relative height of the label as compared to the length of the
   * slider.
   */
  svtkSetClampMacro(LabelHeight, double, 0.0, 2.0);
  svtkGetMacro(LabelHeight, double);
  //@}

  //@{
  /**
   * Specify the relative height of the title as compared to the length of the
   * slider.
   */
  svtkSetClampMacro(TitleHeight, double, 0.0, 2.0);
  svtkGetMacro(TitleHeight, double);
  //@}

  //@{
  /**
   * Indicate whether the slider text label should be displayed. This is
   * a number corresponding to the current Value of this widget.
   */
  svtkSetMacro(ShowSliderLabel, svtkTypeBool);
  svtkGetMacro(ShowSliderLabel, svtkTypeBool);
  svtkBooleanMacro(ShowSliderLabel, svtkTypeBool);
  //@}

  /**
   * Methods to interface with the svtkSliderWidget. Subclasses of this class
   * actually do something.
   */
  virtual double GetCurrentT() { return this->CurrentT; }
  virtual double GetPickedT() { return this->PickedT; }

  // Enums are used to describe what is selected
  enum _InteractionState
  {
    Outside = 0,
    Tube,
    LeftCap,
    RightCap,
    Slider
  };

protected:
  svtkSliderRepresentation();
  ~svtkSliderRepresentation() override;

  // Values
  double Value;
  double MinimumValue;
  double MaximumValue;

  // More ivars controlling the appearance of the widget
  double SliderLength;
  double SliderWidth;
  double EndCapLength;
  double EndCapWidth;
  double TubeWidth;

  // The current parametric coordinate
  double CurrentT;
  double PickedT;

  // both the title and label
  svtkTypeBool ShowSliderLabel;
  char* LabelFormat;
  double LabelHeight;
  double TitleHeight;

private:
  svtkSliderRepresentation(const svtkSliderRepresentation&) = delete;
  void operator=(const svtkSliderRepresentation&) = delete;
};

#endif
