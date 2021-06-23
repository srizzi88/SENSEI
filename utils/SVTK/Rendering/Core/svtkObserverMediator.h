/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkObserverMediator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkObserverMediator
 * @brief   manage contention for cursors and other resources
 *
 * The svtkObserverMediator is a helper class that manages requests for
 * cursor changes from multiple interactor observers (e.g. widgets). It keeps
 * a list of widgets (and their priorities) and their current requests for
 * cursor shape. It then satisfies requests based on widget priority and the
 * relative importance of the request (e.g., a lower priority widget
 * requesting a particular cursor shape will overrule a higher priority
 * widget requesting a default shape).
 *
 * @sa
 * svtkAbstractWidget svtkWidgetRepresentation
 */

#ifndef svtkObserverMediator_h
#define svtkObserverMediator_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderWindowInteractor;
class svtkInteractorObserver;
class svtkObserverMap;

class SVTKRENDERINGCORE_EXPORT svtkObserverMediator : public svtkObject
{
public:
  /**
   * Instantiate the class.
   */
  static svtkObserverMediator* New();

  //@{
  /**
   * Standard macros.
   */
  svtkTypeMacro(svtkObserverMediator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the instance of svtkRenderWindow whose cursor shape is
   * to be managed.
   */
  void SetInteractor(svtkRenderWindowInteractor* iren);
  svtkGetObjectMacro(Interactor, svtkRenderWindowInteractor);
  //@}

  /**
   * Method used to request a cursor shape. Note that the shape is specified
   * using one of the integral values determined in svtkRenderWindow.h. The
   * method returns a non-zero value if the shape was successfully changed.
   */
  int RequestCursorShape(svtkInteractorObserver*, int cursorShape);

  /**
   * Remove all requests for cursor shape from a given interactor.
   */
  void RemoveAllCursorShapeRequests(svtkInteractorObserver*);

protected:
  svtkObserverMediator();
  ~svtkObserverMediator() override;

  // The render window whose cursor we are controlling
  svtkRenderWindowInteractor* Interactor;

  // A map whose key is the observer*, and whose data value is a cursor
  // request. Note that a special compare function is used to sort the
  // widgets based on the observer's priority.
  svtkObserverMap* ObserverMap; // given a widget*, return its data

  // The current observer and cursor shape
  svtkInteractorObserver* CurrentObserver;
  int CurrentCursorShape;

private:
  svtkObserverMediator(const svtkObserverMediator&) = delete;
  void operator=(const svtkObserverMediator&) = delete;
};

#endif
