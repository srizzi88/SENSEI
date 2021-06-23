/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractiveArea.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkInteractiveArea
 * @brief   Implements zooming and panning in a svtkContextArea.
 *
 * Implements zooming and panning in a svtkContextArea.
 */

#ifndef svtkInteractiveArea_h
#define svtkInteractiveArea_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextArea.h"
#include "svtkNew.h" // For svtkNew

class svtkContextTransform;
class svtkRectd;

class SVTKCHARTSCORE_EXPORT svtkInteractiveArea : public svtkContextArea
{
public:
  svtkTypeMacro(svtkInteractiveArea, svtkContextArea);

  static svtkInteractiveArea* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  ///@{
  /**
   * \brief svtkAbstractContextItem API
   */
  bool Paint(svtkContext2D* painter) override;
  bool Hit(const svtkContextMouseEvent& mouse) override;
  bool MouseWheelEvent(const svtkContextMouseEvent& mouse, int delta) override;
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;
  ///@}

protected:
  svtkInteractiveArea();
  ~svtkInteractiveArea() override;

  ///@{
  /**
   * \brief svtkContextArea API
   */
  void SetAxisRange(svtkRectd const& data) override;

private:
  /**
   * Re-scale axis when interacting.
   */
  void RecalculateTickSpacing(svtkAxis* axis, int const numClicks);

  /**
   * Re-computes the transformation expressing the current zoom, panning, etc.
   */
  void ComputeViewTransform() override;

  void ComputeZoom(
    svtkVector2d const& origin, svtkVector2d& scale, svtkVector2d& shift, svtkVector2d& factor);

  class MouseActions;
  MouseActions* Actions;

  svtkInteractiveArea(const svtkInteractiveArea&) = delete;
  void operator=(const svtkInteractiveArea&) = delete;
};

#endif // svtkInteractiveArea_h
