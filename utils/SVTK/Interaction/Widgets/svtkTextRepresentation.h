/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextRepresentation
 * @brief   represent text for svtkTextWidget
 *
 * This class represents text for a svtkTextWidget.  This class provides
 * support for interactively placing text on the 2D overlay plane. The text
 * is defined by an instance of svtkTextActor.
 *
 * @sa
 * svtkTextRepresentation svtkBorderWidget svtkAbstractWidget svtkWidgetRepresentation
 */

#ifndef svtkTextRepresentation_h
#define svtkTextRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkRenderer;
class svtkTextActor;
class svtkTextProperty;
class svtkTextRepresentationObserver;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTextRepresentation : public svtkBorderRepresentation
{
public:
  /**
   * Instantiate class.
   */
  static svtkTextRepresentation* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkTextRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the svtkTextActor to manage. If not specified, then one
   * is automatically created.
   */
  void SetTextActor(svtkTextActor* textActor);
  svtkGetObjectMacro(TextActor, svtkTextActor);
  //@}

  //@{
  /**
   * Get/Set the text string display by this representation.
   */
  void SetText(const char* text);
  const char* GetText();
  //@}

  /**
   * Satisfy the superclasses API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
  {
    size[0] = 2.0;
    size[1] = 2.0;
  }

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  enum
  {
    AnyLocation = 0,
    LowerLeftCorner,
    LowerRightCorner,
    LowerCenter,
    UpperLeftCorner,
    UpperRightCorner,
    UpperCenter
  };

  //@{
  /**
   * Set the text position, by enumeration (
   * AnyLocation = 0,
   * LowerLeftCorner,
   * LowerRightCorner,
   * LowerCenter,
   * UpperLeftCorner,
   * UpperRightCorner,
   * UpperCenter)
   * related to the render window
   */
  virtual void SetWindowLocation(int enumLocation);
  svtkGetMacro(WindowLocation, int);
  //@}

  //@{
  /**
   * Set the text position, by overriding the same function of
   * svtkBorderRepresentation so that the Modified() will be called.
   */
  void SetPosition(double x, double y) override;
  void SetPosition(double pos[2]) override { this->SetPosition(pos[0], pos[1]); }
  //@}

  //@{
  /**
   * Internal. Execute events observed by internal observer
   */
  void ExecuteTextPropertyModifiedEvent(svtkObject* obj, unsigned long enumEvent, void* p);
  void ExecuteTextActorModifiedEvent(svtkObject* obj, unsigned long enumEvent, void* p);
  //@}

protected:
  svtkTextRepresentation();
  ~svtkTextRepresentation() override;

  // Initialize text actor
  virtual void InitializeTextActor();

  // Check and adjust boundaries according to the size of the text
  virtual void CheckTextBoundary();

  // the text to manage
  svtkTextActor* TextActor;
  svtkTextProperty* TextProperty;

  // Window location by enumeration
  int WindowLocation;
  virtual void UpdateWindowLocation();

  // observer to observe internal TextActor and TextProperty
  svtkTextRepresentationObserver* Observer;

private:
  svtkTextRepresentation(const svtkTextRepresentation&) = delete;
  void operator=(const svtkTextRepresentation&) = delete;
};

#endif
