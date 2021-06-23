/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHandleRepresentation
 * @brief   abstract class for representing widget handles
 *
 * This class defines an API for widget handle representations. These
 * representations interact with svtkHandleWidget. Various representations
 * can be used depending on the nature of the handle. The basic functionality
 * of the handle representation is to maintain a position. The position is
 * represented via a svtkCoordinate, meaning that the position can be easily
 * obtained in a variety of coordinate systems.
 *
 * Optional features for this representation include an active mode (the widget
 * appears only when the mouse pointer is close to it). The active distance is
 * expressed in pixels and represents a circle in display space.
 *
 * The class may be subclassed so that alternative representations can
 * be created.  The class defines an API and a default implementation that
 * the svtkHandleWidget interacts with to render itself in the scene.
 *
 * @warning
 * The separation of the widget event handling and representation enables
 * users and developers to create new appearances for the widget. It also
 * facilitates parallel processing, where the client application handles
 * events, and remote representations of the widget are slaves to the
 * client (and do not handle events).
 *
 * @sa
 * svtkRectilinearWipeWidget svtkWidgetRepresentation svtkAbstractWidget
 */

#ifndef svtkHandleRepresentation_h
#define svtkHandleRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkCoordinate;
class svtkRenderer;
class svtkPointPlacer;

class SVTKINTERACTIONWIDGETS_EXPORT svtkHandleRepresentation : public svtkWidgetRepresentation
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkHandleRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Handles usually have their coordinates set in display coordinates
   * (generally by an associated widget) and internally maintain the position
   * in world coordinates. (Using world coordinates insures that handles are
   * rendered in the right position when the camera view changes.) These
   * methods are often subclassed because special constraint operations can
   * be used to control the actual positioning.
   */
  virtual void SetDisplayPosition(double pos[3]);
  virtual void GetDisplayPosition(double pos[3]);
  virtual double* GetDisplayPosition() SVTK_SIZEHINT(3);
  virtual void SetWorldPosition(double pos[3]);
  virtual void GetWorldPosition(double pos[3]);
  virtual double* GetWorldPosition() SVTK_SIZEHINT(3);
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels)
   * in which the cursor is considered near enough to the widget to
   * be active.
   */
  svtkSetClampMacro(Tolerance, int, 1, 100);
  svtkGetMacro(Tolerance, int);
  //@}

  //@{
  /**
   * Flag controls whether the widget becomes visible when the mouse pointer
   * moves close to it (i.e., the widget becomes active). By default,
   * ActiveRepresentation is off and the representation is always visible.
   */
  svtkSetMacro(ActiveRepresentation, svtkTypeBool);
  svtkGetMacro(ActiveRepresentation, svtkTypeBool);
  svtkBooleanMacro(ActiveRepresentation, svtkTypeBool);
  //@}

  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget. Note that ComputeInteractionState() and several other methods
  // must be implemented by subclasses.
  enum _InteractionState
  {
    Outside = 0,
    Nearby,
    Selecting,
    Translating,
    Scaling
  };

  //@{
  /**
   * The interaction state may be set from a widget (e.g., HandleWidget) or
   * other object. This controls how the interaction with the widget
   * proceeds. Normally this method is used as part of a handshaking
   * processwith the widget: First ComputeInteractionState() is invoked that
   * returns a state based on geometric considerations (i.e., cursor near a
   * widget feature), then based on events, the widget may modify this
   * further.
   */
  svtkSetClampMacro(InteractionState, int, Outside, Scaling);
  //@}

  //@{
  /**
   * Specify whether any motions (such as scale, translate, etc.) are
   * constrained in some way (along an axis, etc.) Widgets can use this
   * to control the resulting motion.
   */
  svtkSetMacro(Constrained, svtkTypeBool);
  svtkGetMacro(Constrained, svtkTypeBool);
  svtkBooleanMacro(Constrained, svtkTypeBool);
  //@}

  /**
   * Method has to be overridden in the subclasses which has
   * constraints on placing the handle
   * (Ex. svtkConstrainedPointHandleRepresentation). It should return 1
   * if the position is within the constraint, else it should return
   * 0. By default it returns 1.
   */
  virtual int CheckConstraint(svtkRenderer* renderer, double pos[2]);

  //@{
  /**
   * Methods to make this class properly act like a svtkWidgetRepresentation.
   */
  void ShallowCopy(svtkProp* prop) override;
  virtual void DeepCopy(svtkProp* prop);
  void SetRenderer(svtkRenderer* ren) override;
  //@}

  /**
   * Overload the superclasses' GetMTime() because the internal svtkCoordinates
   * are used to keep the state of the representation.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the point placer. Point placers can be used to dictate constraints
   * on the placement of handles. As an example, see svtkBoundedPlanePointPlacer
   * (constrains the placement of handles to a set of bounded planes)
   * svtkFocalPlanePointPlacer (constrains placement on the focal plane) etc.
   * The default point placer is svtkPointPlacer (which does not apply any
   * constraints, so the handles are free to move anywhere).
   */
  virtual void SetPointPlacer(svtkPointPlacer*);
  svtkGetObjectMacro(PointPlacer, svtkPointPlacer);
  //@}

  //@{
  /**
   * Gets the translation vector
   */
  virtual void GetTranslationVector(const double* p1, const double* p2, double* v) const;

  //@{
  /**
   * Translates world position by vector p1p2 projected on the constraint axis if any.
   */
  virtual void Translate(const double* p1, const double* p2);
  //@}

  //@{
  /**
   * Translates world position by vector v projected on the constraint axis if any.
   */
  virtual void Translate(const double* v);
  //@}

  //@{
  /**
   * Gets/Sets the constraint axis for translations. Returns Axis::NONE
   * if none.
   **/
  svtkGetMacro(TranslationAxis, int);
  svtkSetClampMacro(TranslationAxis, int, -1, 2);
  //@}

  //@{
  /**
   * Toggles constraint translation axis on/off.
   */
  void SetXTranslationAxisOn() { this->TranslationAxis = Axis::XAxis; }
  void SetYTranslationAxisOn() { this->TranslationAxis = Axis::YAxis; }
  void SetZTranslationAxisOn() { this->TranslationAxis = Axis::ZAxis; }
  void SetTranslationAxisOff() { this->TranslationAxis = Axis::NONE; }
  //@}

  //@{
  /**
   * Returns true if ContrainedAxis
   **/
  bool IsTranslationConstrained() { return this->TranslationAxis != Axis::NONE; }
  //@}

protected:
  svtkHandleRepresentation();
  ~svtkHandleRepresentation() override;

  int Tolerance;
  svtkTypeBool ActiveRepresentation;
  svtkTypeBool Constrained;

  // Two svtkCoordinates are available to subclasses, one in display
  // coordinates and the other in world coordinates. These facilitate
  // the conversion between these two systems. Note that the WorldPosition
  // is the ultimate maintainer of position.
  svtkCoordinate* DisplayPosition;
  svtkCoordinate* WorldPosition;

  // Keep track of when coordinates were changed
  svtkTimeStamp DisplayPositionTime;
  svtkTimeStamp WorldPositionTime;

  // Constraint the placement of handles.
  svtkPointPlacer* PointPlacer;

  // Constraint axis translation
  int TranslationAxis;

private:
  svtkHandleRepresentation(const svtkHandleRepresentation&) = delete;
  void operator=(const svtkHandleRepresentation&) = delete;
};

#endif
