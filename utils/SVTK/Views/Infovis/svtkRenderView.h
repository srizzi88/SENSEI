/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderView.h

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
 * @class   svtkRenderView
 * @brief   A view containing a renderer.
 *
 *
 * svtkRenderView is a view which contains a svtkRenderer.  You may add svtkActors
 * directly to the renderer, or add certain svtkDataRepresentation subclasses
 * to the renderer.  The render view supports drag selection with the mouse to
 * select cells.
 *
 * This class is also the parent class for any more specialized view which uses
 * a renderer.
 */

#ifndef svtkRenderView_h
#define svtkRenderView_h

#include "svtkRenderViewBase.h"
#include "svtkSmartPointer.h"       // For SP ivars
#include "svtkViewsInfovisModule.h" // For export macro

class svtkAbstractTransform;
class svtkActor2D;
class svtkAlgorithmOutput;
class svtkArrayCalculator;
class svtkBalloonRepresentation;
class svtkDynamic2DLabelMapper;
class svtkHardwareSelector;
class svtkHoverWidget;
class svtkInteractorObserver;
class svtkLabelPlacementMapper;
class svtkPolyDataMapper2D;
class svtkSelection;
class svtkTextProperty;
class svtkTexture;
class svtkTexturedActor2D;
class svtkTransformCoordinateSystems;

class SVTKVIEWSINFOVIS_EXPORT svtkRenderView : public svtkRenderViewBase
{
public:
  static svtkRenderView* New();
  svtkTypeMacro(svtkRenderView, svtkRenderViewBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The render window interactor. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of svtkRenderViewBase.
   */
  void SetInteractor(svtkRenderWindowInteractor* interactor) override;

  /**
   * The interactor style associated with the render view.
   */
  virtual void SetInteractorStyle(svtkInteractorObserver* style);

  /**
   * Get the interactor style associated with the render view.
   */
  virtual svtkInteractorObserver* GetInteractorStyle();

  /**
   * Set the render window for this view. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of svtkRenderViewBase.
   */
  void SetRenderWindow(svtkRenderWindow* win) override;

  enum
  {
    INTERACTION_MODE_2D,
    INTERACTION_MODE_3D,
    INTERACTION_MODE_UNKNOWN
  };

  void SetInteractionMode(int mode);
  svtkGetMacro(InteractionMode, int);

  /**
   * Set the interaction mode for the view. Choices are:
   * svtkRenderView::INTERACTION_MODE_2D - 2D interactor
   * svtkRenderView::INTERACTION_MODE_3D - 3D interactor
   */
  virtual void SetInteractionModeTo2D() { this->SetInteractionMode(INTERACTION_MODE_2D); }
  virtual void SetInteractionModeTo3D() { this->SetInteractionMode(INTERACTION_MODE_3D); }

  /**
   * Updates the representations, then calls Render() on the render window
   * associated with this view.
   */
  void Render() override;

  /**
   * Applies a view theme to this view.
   */
  void ApplyViewTheme(svtkViewTheme* theme) override;

  //@{
  /**
   * Set the view's transform. All svtkRenderedRepresentations
   * added to this view should use this transform.
   */
  virtual void SetTransform(svtkAbstractTransform* transform);
  svtkGetObjectMacro(Transform, svtkAbstractTransform);
  //@}

  //@{
  /**
   * Whether the view should display hover text.
   */
  virtual void SetDisplayHoverText(bool b);
  svtkGetMacro(DisplayHoverText, bool);
  svtkBooleanMacro(DisplayHoverText, bool);
  //@}

  enum
  {
    SURFACE = 0,
    FRUSTUM = 1
  };

  //@{
  /**
   * Sets the selection mode for the render view.
   * SURFACE selection uses svtkHardwareSelector to perform a selection
   * of visible cells.
   * FRUSTUM selection just creates a view frustum selection, which will
   * select everything in the frustum.
   */
  svtkSetClampMacro(SelectionMode, int, 0, 1);
  svtkGetMacro(SelectionMode, int);
  void SetSelectionModeToSurface() { this->SetSelectionMode(SURFACE); }
  void SetSelectionModeToFrustum() { this->SetSelectionMode(FRUSTUM); }
  //@}

  /**
   * Add labels from an input connection with an associated text
   * property. The output must be a svtkLabelHierarchy (normally the
   * output of svtkPointSetToLabelHierarchy).
   */
  virtual void AddLabels(svtkAlgorithmOutput* conn);

  /**
   * Remove labels from an input connection.
   */
  virtual void RemoveLabels(svtkAlgorithmOutput* conn);

  //@{
  /**
   * Set the icon sheet to use for rendering icons.
   */
  virtual void SetIconTexture(svtkTexture* texture);
  svtkGetObjectMacro(IconTexture, svtkTexture);
  //@}

  //@{
  /**
   * Set the size of each icon in the icon texture.
   */
  svtkSetVector2Macro(IconSize, int);
  svtkGetVector2Macro(IconSize, int);
  //@}

  //@{
  /**
   * Set the display size of the icon (which may be different from the icon
   * size). By default, if this value is not set, the IconSize is used.
   */
  svtkSetVector2Macro(DisplaySize, int);
  int* GetDisplaySize();
  void GetDisplaySize(int& dsx, int& dsy);
  //@}

  enum
  {
    NO_OVERLAP,
    ALL
  };

  //@{
  /**
   * Label placement mode.
   * NO_OVERLAP uses svtkLabelPlacementMapper, which has a faster startup time and
   * works with 2D or 3D labels.
   * ALL displays all labels (Warning: This may cause incredibly slow render
   * times on datasets with more than a few hundred labels).
   */
  virtual void SetLabelPlacementMode(int mode);
  virtual int GetLabelPlacementMode();
  virtual void SetLabelPlacementModeToNoOverlap() { this->SetLabelPlacementMode(NO_OVERLAP); }
  virtual void SetLabelPlacementModeToAll() { this->SetLabelPlacementMode(ALL); }
  //@}

  enum
  {
    FREETYPE,
    QT
  };

  //@{
  /**
   * Label render mode.
   * FREETYPE uses the freetype label rendering.
   * QT uses more advanced Qt-based label rendering.
   */
  virtual void SetLabelRenderMode(int mode);
  virtual int GetLabelRenderMode();
  virtual void SetLabelRenderModeToFreetype() { this->SetLabelRenderMode(FREETYPE); }
  virtual void SetLabelRenderModeToQt() { this->SetLabelRenderMode(QT); }
  //@}

  //@{
  /**
   * Whether to render on every mouse move.
   */
  void SetRenderOnMouseMove(bool b);
  svtkGetMacro(RenderOnMouseMove, bool);
  svtkBooleanMacro(RenderOnMouseMove, bool);
  //@}

protected:
  svtkRenderView();
  ~svtkRenderView() override;

  /**
   * Called to process events.
   * Captures StartEvent events from the renderer and calls Update().
   * This may be overridden by subclasses to process additional events.
   */
  void ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData) override;

  /**
   * Generates the selection based on the view event and the selection mode.
   */
  virtual void GenerateSelection(void* callData, svtkSelection* selection);

  /**
   * Called by the view when the renderer is about to render.
   */
  void PrepareForRendering() override;

  /**
   * Called in PrepareForRendering to update the hover text.
   */
  virtual void UpdateHoverText();

  /**
   * Enable or disable hovering based on DisplayHoverText ivar
   * and interaction state.
   */
  virtual void UpdateHoverWidgetState();

  /**
   * Update the pick render for queries for drag selections
   * or hover ballooons.
   */
  void UpdatePickRender();

  int SelectionMode;
  int LabelRenderMode;
  bool DisplayHoverText;
  bool Interacting;
  bool InHoverTextRender;
  bool InPickRender;
  bool PickRenderNeedsUpdate;

  svtkAbstractTransform* Transform;
  svtkTexture* IconTexture;
  int IconSize[2];
  int DisplaySize[2];

  int InteractionMode;
  bool RenderOnMouseMove;

  svtkSmartPointer<svtkRenderer> LabelRenderer;
  svtkSmartPointer<svtkBalloonRepresentation> Balloon;
  svtkSmartPointer<svtkLabelPlacementMapper> LabelPlacementMapper;
  svtkSmartPointer<svtkTexturedActor2D> LabelActor;
  svtkSmartPointer<svtkHoverWidget> HoverWidget;
  svtkSmartPointer<svtkHardwareSelector> Selector;

private:
  svtkRenderView(const svtkRenderView&) = delete;
  void operator=(const svtkRenderView&) = delete;
};

#endif
