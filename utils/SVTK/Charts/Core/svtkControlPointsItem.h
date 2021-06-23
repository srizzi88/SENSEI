/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkControlPointsItem
 * @brief   Abstract class for control points items.
 *
 * svtkControlPointsItem provides control point painting and management for
 * subclasses that provide points (typically control points of a transfer
 * function)
 * @sa
 * svtkScalarsToColorsItem
 * svtkPiecewiseControlPointsItem
 */

#ifndef svtkControlPointsItem_h
#define svtkControlPointsItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkCommand.h"          // For svtkCommand enum
#include "svtkPlot.h"
#include "svtkSmartPointer.h" // for SmartPointer
#include "svtkVector.h"       // For svtkVector2f

class svtkCallbackCommand;
class svtkContext2D;
class svtkControlPointsAddPointItem;
class svtkPiecewisePointHandleItem;
class svtkPoints2D;
class svtkTransform2D;

class SVTKCHARTSCORE_EXPORT svtkControlPointsItem : public svtkPlot
{
public:
  svtkTypeMacro(svtkControlPointsItem, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Events fires by this class (and subclasses).
  // \li CurrentPointChangedEvent is fired when the current point index is changed.
  // \li CurrentPointEditEvent is fired to request the application to show UI to
  // edit the current point.
  // \li svtkCommand::StartEvent and svtkCommand::EndEvent is fired
  // to mark groups of changes to control points.
  enum
  {
    CurrentPointChangedEvent = svtkCommand::UserEvent,
    CurrentPointEditEvent
  };

  /**
   * Bounds of the item, typically the bound of all the control points
   * except if custom bounds have been set \sa SetUserBounds.
   */
  void GetBounds(double bounds[4]) override;

  //@{
  /**
   * Set custom bounds, except if bounds are invalid, bounds will be
   * automatically computed based on the range of the control points
   * Invalid bounds by default.
   */
  svtkSetVector4Macro(UserBounds, double);
  svtkGetVector4Macro(UserBounds, double);
  //@}

  //@{
  /**
   * Controls the valid range for the values.
   * An invalid value (0, -1, 0., -1, 0, -1.) indicates that the valid
   * range is the current bounds. It is the default behavior.
   */
  svtkSetVector4Macro(ValidBounds, double);
  svtkGetVector4Macro(ValidBounds, double);
  //@}

  //@{
  /**
   * Get/set the radius for screen points.
   * Default is 6.f
   */
  svtkGetMacro(ScreenPointRadius, float);
  svtkSetMacro(ScreenPointRadius, float);
  //@}

  /**
   * Paint the points with a fixed size (cosmetic) which doesn't depend
   * on the scene zoom factor. Selected and unselected points are drawn
   * with a different color.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Select a point by its ID
   */
  void SelectPoint(svtkIdType pointId);

  /**
   * Utility function that selects a point providing its coordinates.
   * To be found, the position of the point must be no further away than its
   * painted point size
   */
  void SelectPoint(double* currentPoint);

  /**
   * Select all the points
   */
  void SelectAllPoints();

  /**
   * Unselect a point by its ID
   */
  void DeselectPoint(svtkIdType pointId);

  /**
   * Utility function that unselects a point providing its coordinates.
   * To be found, the position of the point must be no further away than its
   * painted point size
   */
  void DeselectPoint(double* currentPoint);

  /**
   * Unselect all the previously selected points
   */
  void DeselectAllPoints();

  /**
   * Toggle the selection of a point by its ID. If the point was selected then
   * unselect it, otherwise select it.
   */
  void ToggleSelectPoint(svtkIdType pointId);

  /**
   * Utility function that toggles the selection a point providing its
   * coordinates. To be found, the position of the point must be no further
   * away than its painted point size
   */
  void ToggleSelectPoint(double* currentPoint);

  /**
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const svtkVector2f& min, const svtkVector2f& max) override;

  /**
   * Return the number of selected points.
   */
  svtkIdType GetNumberOfSelectedPoints() const;

  /**
   * Returns the svtkIdType of the point given its coordinates and a tolerance
   * based on the screen point size.
   */
  svtkIdType FindPoint(double* pos);

  /**
   * Returns true if pos is above the pointId point, false otherwise.
   * It uses the size of the drawn point. To search what point is under the pos,
   * use the more efficient \sa FindPoint() instead.
   */
  bool IsOverPoint(double* pos, svtkIdType pointId);

  /**
   * Returns the id of the control point exactly matching pos, -1 if not found.
   */
  svtkIdType GetControlPointId(double* pos);

  /**
   * Utility function that returns an array of all the control points IDs
   * Typically: [0, 1, 2, ... n -1] where n is the point count
   * Can exclude the first and last point ids from the array.
   */
  void GetControlPointsIds(svtkIdTypeArray* ids, bool excludeFirstAndLast = false) const;

  //@{
  /**
   * Controls whether or not control points are drawn (true) or clicked and
   * moved (false).
   * False by default.
   */
  svtkGetMacro(StrokeMode, bool);
  //@}

  //@{
  /**
   * If DrawPoints is false, SwitchPoints controls the behavior when a control
   * point is dragged past another point. The crossed point becomes current
   * (true) or the current point is blocked/stopped (false).
   * False by default.
   */
  svtkSetMacro(SwitchPointsMode, bool);
  svtkGetMacro(SwitchPointsMode, bool);
  //@}

  //@{
  /**
   * If EndPointsMovable is false, the two end points will not
   * be moved. True by default.
   */
  svtkSetMacro(EndPointsXMovable, bool);
  svtkGetMacro(EndPointsXMovable, bool);
  svtkSetMacro(EndPointsYMovable, bool);
  svtkGetMacro(EndPointsYMovable, bool);
  virtual bool GetEndPointsMovable();
  //@}

  //@{
  /**
   * If EndPointsRemovable is false, the two end points will not
   * be removed. True by default.
   */
  svtkSetMacro(EndPointsRemovable, bool);
  svtkGetMacro(EndPointsRemovable, bool);
  //@}

  //@{
  /**
   * When set to true, labels are shown on the current control point and the end
   * points. Default is false.
   */
  svtkSetMacro(ShowLabels, bool);
  svtkGetMacro(ShowLabels, bool);
  //@}

  //@{
  /**
   * Get/Set the label format. Default is "%.4f, %.4f".
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  virtual svtkIdType AddPoint(double* newPos) = 0;

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  virtual svtkIdType RemovePoint(double* pos) = 0;

  /**
   * Remove a point give its id. It is a utility function that internally call
   * the virtual method RemovePoint(double*) and return its result.
   */
  svtkIdType RemovePoint(svtkIdType pointId);

  /**
   * Remove the current point.
   */
  inline void RemoveCurrentPoint();

  /**
   * Returns the total number of points
   */
  virtual svtkIdType GetNumberOfPoints() const = 0;

  /**
   * Returns the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * point must be a double array of size 4.
   */
  virtual void GetControlPoint(svtkIdType index, double* point) const = 0;

  /**
   * Sets the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   */
  virtual void SetControlPoint(svtkIdType index, double* point) = 0;

  /**
   * Move the points referred by pointIds by a given translation.
   * The new positions won't be outside the bounds.
   * MovePoints is typically called with GetControlPointsIds() or GetSelection().
   * Warning: if you pass this->GetSelection(), the array is deleted after
   * each individual point move. Increase the reference count of the array.
   * See also MoveAllPoints()
   */
  void MovePoints(const svtkVector2f& translation, svtkIdTypeArray* pointIds);

  /**
   * Utility function to move all the control points of the given translation
   * If dontMoveFirstAndLast is true, then the first and last points won't be
   * moved.
   */
  void MovePoints(const svtkVector2f& translation, bool dontMoveFirstAndLast = false);

  /**
   * Spread the points referred by pointIds
   * If factor > 0, points are moved away from each other.
   * If factor < 0, points are moved closer to each other
   * SpreadPoints is typically called with GetControlPointsIds() or GetSelection().
   * Warning: if you pass this->GetSelection(), the array is deleted after
   * each individual point move. Increase the reference count of the array.
   */
  void SpreadPoints(float factor, svtkIdTypeArray* pointIds);

  /**
   * Utility function to spread all the control points of a given factor
   * If dontSpreadFirstAndLast is true, then the first and last points won't be
   * spread.
   */
  void SpreadPoints(float factor, bool dontSpreadFirstAndLast = false);

  /**
   * Returns the current point ID selected or -1 if there is no point current.
   * No current point by default.
   */
  svtkIdType GetCurrentPoint() const;

  /**
   * Sets the current point selected.
   */
  void SetCurrentPoint(svtkIdType index);

  //@{
  /**
   * Gets the selected point pen and brush.
   */
  svtkGetObjectMacro(SelectedPointPen, svtkPen);
  //@}

  //@{
  /**
   * Depending on the control points item, the brush might not be taken into
   * account.
   */
  svtkGetObjectMacro(SelectedPointBrush, svtkBrush);
  //@}

  //@{
  /**
   * When enabled, a dedicated item is used to determine if a point should
   * be added when clicking anywhere.
   * This item can be recovered with GetAddPointItem and can this be placed
   * below all other items. False by default.
   */
  svtkGetMacro(UseAddPointItem, bool);
  svtkSetMacro(UseAddPointItem, bool);
  svtkBooleanMacro(UseAddPointItem, bool);
  //@}

  /**
   * Item dedicated to add point, to be added below all other items.
   * Used only if UseAddPointItem is set to true.
   */
  svtkPlot* GetAddPointItem();

  /**
   * Recompute the bounds next time they are requested.
   * You shouldn't have to call it but it is provided for rare cases.
   */
  void ResetBounds();

  //@{
  /**
   * Mouse and key events.
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseDoubleClickEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;
  bool KeyPressEvent(const svtkContextKeyEvent& key) override;
  bool KeyReleaseEvent(const svtkContextKeyEvent& key) override;
  //@}

protected:
  svtkControlPointsItem();
  ~svtkControlPointsItem() override;

  friend class svtkPiecewisePointHandleItem;

  void StartChanges();
  void EndChanges();
  void StartInteraction();
  void StartInteractionIfNotStarted();
  void Interaction();
  void EndInteraction();
  int GetInteractionsCount() const;
  virtual void emitEvent(unsigned long event, void* params = nullptr) = 0;

  static void CallComputePoints(
    svtkObject* sender, unsigned long event, void* receiver, void* params);

  //@{
  /**
   * Must be reimplemented by subclasses to calculate the points to draw.
   * It's subclass responsibility to call ComputePoints() via the callback
   */
  virtual void ComputePoints();
  virtual svtkMTimeType GetControlPointsMTime() = 0;
  //@}

  /**
   * Returns true if the supplied x, y are within the bounds or on a control point.
   * If UseAddPointItem is true,
   * returns true only if the supplied x, y are on a control point.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  //@{
  /**
   * Clamp the given 2D pos into the bounds of the function.
   * Return true if the pos has been clamped, false otherwise.
   */
  bool ClampValidDataPos(double pos[2]);
  bool ClampValidScreenPos(double pos[2]);
  //@}

  //@{
  /**
   * Internal function that paints a collection of points and optionally
   * excludes some.
   */
  void DrawUnselectedPoints(svtkContext2D* painter);
  void DrawSelectedPoints(svtkContext2D* painter);
  virtual void DrawPoint(svtkContext2D* painter, svtkIdType index);
  //@}

  void SetCurrentPointPos(const svtkVector2f& newPos);
  svtkIdType SetPointPos(svtkIdType point, const svtkVector2f& newPos);
  void MoveCurrentPoint(const svtkVector2f& translation);
  svtkIdType MovePoint(svtkIdType point, const svtkVector2f& translation);

  inline svtkVector2f GetSelectionCenterOfMass() const;
  svtkVector2f GetCenterOfMass(svtkIdTypeArray* pointIDs) const;

  void Stroke(const svtkVector2f& newPos);
  virtual void EditPoint(float svtkNotUsed(tX), float svtkNotUsed(tY));

  /**
   * Generate label for a control point.
   */
  virtual svtkStdString GetControlPointLabel(svtkIdType index);

  void AddPointId(svtkIdType addedPointId);

  /**
   * Return true if any of the end points is current point
   * or part of the selection
   */
  bool IsEndPointPicked();

  /**
   * Return true if the point is removable
   */
  bool IsPointRemovable(svtkIdType pointId);

  /**
   * Compute the bounds for this item. Typically, the bounds should be aligned
   * to the range of the svtkScalarsToColors or svtkPiecewiseFunction that is
   * being controlled by the subclasses.
   * Default implementation uses the range of the control points themselves.
   */
  virtual void ComputeBounds(double* bounds);

  svtkCallbackCommand* Callback;
  svtkPen* SelectedPointPen;
  svtkBrush* SelectedPointBrush;
  int BlockUpdates;
  int StartedInteractions;
  int StartedChanges;
  svtkIdType CurrentPoint;

  double Bounds[4];
  double UserBounds[4];
  double ValidBounds[4];

  svtkTransform2D* Transform;
  float ScreenPointRadius;

  bool StrokeMode;
  bool SwitchPointsMode;
  bool MouseMoved;
  bool EnforceValidFunction;
  svtkIdType PointToDelete;
  bool PointAboutToBeDeleted;
  svtkIdType PointToToggle;
  bool PointAboutToBeToggled;
  bool InvertShadow;
  bool EndPointsXMovable;
  bool EndPointsYMovable;
  bool EndPointsRemovable;
  bool ShowLabels;
  char* LabelFormat;

private:
  svtkControlPointsItem(const svtkControlPointsItem&) = delete;
  void operator=(const svtkControlPointsItem&) = delete;

  void ComputeBounds();

  svtkIdType RemovePointId(svtkIdType removedPointId);

  bool UseAddPointItem = false;
  svtkNew<svtkControlPointsAddPointItem> AddPointItem;
};

//-----------------------------------------------------------------------------
void svtkControlPointsItem::RemoveCurrentPoint()
{
  this->RemovePoint(this->GetCurrentPoint());
}

//-----------------------------------------------------------------------------
svtkVector2f svtkControlPointsItem::GetSelectionCenterOfMass() const
{
  return this->GetCenterOfMass(this->Selection);
}

#endif
