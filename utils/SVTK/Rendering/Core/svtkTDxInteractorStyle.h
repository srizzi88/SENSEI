/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTDxInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTDxInteractorStyle
 * @brief   provide 3DConnexion device event-driven interface to the rendering window
 *
 *
 * svtkTDxInteractorStyle is an abstract class defining an event-driven
 * interface to support 3DConnexion device events send by
 * svtkRenderWindowInteractor.
 * svtkRenderWindowInteractor forwards events in a platform independent form to
 * svtkInteractorStyle which can then delegate some processing to
 * svtkTDxInteractorStyle.
 *
 * @sa
 * svtkInteractorStyle svtkRenderWindowInteractor
 * svtkTDxInteractorStyleCamera
 */

#ifndef svtkTDxInteractorStyle_h
#define svtkTDxInteractorStyle_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkTDxMotionEventInfo;
class svtkRenderer;
class svtkTDxInteractorStyleSettings;

class SVTKRENDERINGCORE_EXPORT svtkTDxInteractorStyle : public svtkObject
{
public:
  svtkTypeMacro(svtkTDxInteractorStyle, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Action on motion event. Default implementation is empty.
   * \pre: motionInfo_exist: motionInfo!=0
   */
  virtual void OnMotionEvent(svtkTDxMotionEventInfo* motionInfo);

  /**
   * Action on button pressed event. Default implementation is empty.
   */
  virtual void OnButtonPressedEvent(int button);

  /**
   * Action on button released event. Default implementation is empty.
   */
  virtual void OnButtonReleasedEvent(int button);

  /**
   * Dispatch the events TDxMotionEvent, TDxButtonPressEvent and
   * TDxButtonReleaseEvent to OnMotionEvent(), OnButtonPressedEvent() and
   * OnButtonReleasedEvent() respectively.
   * It is called by the svtkInteractorStyle.
   * This method is virtual for convenient but you should really override
   * the On*Event() methods only.
   * \pre renderer can be null.
   */
  virtual void ProcessEvent(svtkRenderer* renderer, unsigned long event, void* calldata);

  //@{
  /**
   * 3Dconnexion device settings. (sensitivity, individual axis filters).
   * Initial object is not null.
   */
  svtkGetObjectMacro(Settings, svtkTDxInteractorStyleSettings);
  virtual void SetSettings(svtkTDxInteractorStyleSettings* settings);
  //@}

protected:
  svtkTDxInteractorStyle();
  ~svtkTDxInteractorStyle() override;

  svtkTDxInteractorStyleSettings* Settings;

  svtkRenderer* Renderer;

private:
  svtkTDxInteractorStyle(const svtkTDxInteractorStyle&) = delete;
  void operator=(const svtkTDxInteractorStyle&) = delete;
};
#endif
