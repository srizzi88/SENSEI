/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceImageViewer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceImageViewer
 * @brief   Display an image along with a reslice cursor
 *
 * This class is similar to svtkImageViewer2. It displays the image along with
 * a two cross hairs for reslicing. The cross hairs may be interactively
 * manipulated and are typically used to reslice two other views of
 * svtkResliceImageViewer. See QtSVTKRenderWindows for an example. The reslice
 * cursor is used to perform thin or thick MPR through data. The class can
 * also default to the behaviour of svtkImageViewer2 if the Reslice mode is
 * set to RESLICE_AXIS_ALIGNED.
 * @sa
 * svtkResliceCursor svtkResliceCursorWidget svtkResliceCursorRepresentation
 */

#ifndef svtkResliceImageViewer_h
#define svtkResliceImageViewer_h

#include "svtkImageViewer2.h"
#include "svtkInteractionImageModule.h" // For export macro

class svtkResliceCursorWidget;
class svtkResliceCursor;
class svtkScalarsToColors;
class svtkBoundedPlanePointPlacer;
class svtkResliceImageViewerMeasurements;
class svtkResliceImageViewerScrollCallback;
class svtkPlane;

class SVTKINTERACTIONIMAGE_EXPORT svtkResliceImageViewer : public svtkImageViewer2
{
public:
  //@{
  /**
   * Standard SVTK methods.
   */
  static svtkResliceImageViewer* New();
  svtkTypeMacro(svtkResliceImageViewer, svtkImageViewer2);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Render the resulting image.
   */
  void Render() override;

  //@{
  /**
   * Set/Get the input image to the viewer.
   */
  void SetInputData(svtkImageData* in) override;
  void SetInputConnection(svtkAlgorithmOutput* input) override;
  //@}

  //@{
  /**
   * Set window and level for mapping pixels to colors.
   */
  void SetColorWindow(double s) override;
  void SetColorLevel(double s) override;
  //@}

  //@{
  /**
   * Get the internal render window, renderer, image actor, and
   * image map instances.
   */
  svtkGetObjectMacro(ResliceCursorWidget, svtkResliceCursorWidget);
  //@}

  /**
   * Set/get the slice orientation
   */

  enum
  {
    RESLICE_AXIS_ALIGNED = 0,
    RESLICE_OBLIQUE = 1
  };

  svtkGetMacro(ResliceMode, int);
  virtual void SetResliceMode(int resliceMode);
  virtual void SetResliceModeToAxisAligned()
  {
    this->SetResliceMode(svtkResliceImageViewer::RESLICE_AXIS_ALIGNED);
  }
  virtual void SetResliceModeToOblique()
  {
    this->SetResliceMode(svtkResliceImageViewer::RESLICE_OBLIQUE);
  }

  //@{
  /**
   * Set/Get the reslice cursor.
   */
  svtkResliceCursor* GetResliceCursor();
  void SetResliceCursor(svtkResliceCursor* rc);
  //@}

  //@{
  /**
   * Set the lookup table
   */
  virtual void SetLookupTable(svtkScalarsToColors*);
  svtkScalarsToColors* GetLookupTable();
  //@}

  //@{
  /**
   * Switch to / from thick mode
   */
  virtual void SetThickMode(int);
  virtual int GetThickMode();
  //@}

  /**
   * Reset all views back to initial state
   */
  virtual void Reset();

  //@{
  /**
   * Get the point placer.
   */
  svtkGetObjectMacro(PointPlacer, svtkBoundedPlanePointPlacer);
  //@}

  //@{
  /**
   * Get the measurements manager
   */
  svtkGetObjectMacro(Measurements, svtkResliceImageViewerMeasurements);
  //@}

  //@{
  /**
   * Get the render window interactor
   */
  svtkGetObjectMacro(Interactor, svtkRenderWindowInteractor);
  //@}

  //@{
  /**
   * Scroll slices on the mouse wheel ? In the case of MPR
   * view, it moves one "normalized spacing" in the direction of the normal to
   * the resliced plane, provided the new center will continue to lie within
   * the volume.
   */
  svtkSetMacro(SliceScrollOnMouseWheel, svtkTypeBool);
  svtkGetMacro(SliceScrollOnMouseWheel, svtkTypeBool);
  svtkBooleanMacro(SliceScrollOnMouseWheel, svtkTypeBool);
  //@}

  /**
   * Increment/Decrement slice by 'n' slices
   */
  virtual void IncrementSlice(int n);

  enum
  {
    SliceChangedEvent = 1001
  };

protected:
  svtkResliceImageViewer();
  ~svtkResliceImageViewer() override;

  void InstallPipeline() override;
  void UnInstallPipeline() override;
  void UpdateOrientation() override;
  void UpdateDisplayExtent() override;
  virtual void UpdatePointPlacer();

  //@{
  /**
   * Convenience methods to get the reslice plane and the normalized
   * spacing between slices in reslice mode.
   */
  svtkPlane* GetReslicePlane();
  double GetInterSliceSpacingInResliceMode();
  //@}

  svtkResliceCursorWidget* ResliceCursorWidget;
  svtkBoundedPlanePointPlacer* PointPlacer;
  int ResliceMode;
  svtkResliceImageViewerMeasurements* Measurements;
  svtkTypeBool SliceScrollOnMouseWheel;
  svtkResliceImageViewerScrollCallback* ScrollCallback;

private:
  svtkResliceImageViewer(const svtkResliceImageViewer&) = delete;
  void operator=(const svtkResliceImageViewer&) = delete;
};

#endif
