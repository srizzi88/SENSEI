/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlaybackWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPlaybackWidget
 * @brief   2D widget for controlling a playback stream
 *
 * This class provides support for interactively controlling the playback of
 * a serial stream of information (e.g., animation sequence, video, etc.).
 * Controls for play, stop, advance one step forward, advance one step backward,
 * jump to beginning, and jump to end are available.
 *
 * @sa
 * svtkBorderWidget
 */

#ifndef svtkPlaybackWidget_h
#define svtkPlaybackWidget_h

#include "svtkBorderWidget.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkPlaybackRepresentation;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPlaybackWidget : public svtkBorderWidget
{
public:
  /**
   * Instantiate this class.
   */
  static svtkPlaybackWidget* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkPlaybackWidget, svtkBorderWidget);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Specify an instance of svtkPlaybackRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of svtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(svtkPlaybackRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

protected:
  svtkPlaybackWidget();
  ~svtkPlaybackWidget() override;

  /**
   * When selecting the interior of this widget, special operations occur
   * (i.e., operating the playback controls).
   */
  void SelectRegion(double eventPos[2]) override;

private:
  svtkPlaybackWidget(const svtkPlaybackWidget&) = delete;
  void operator=(const svtkPlaybackWidget&) = delete;
};

#endif
