/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMapper
 * @brief   2D image display
 *
 * svtkImageMapper provides 2D image display support for svtk.
 * It is a Mapper2D subclass that can be associated with an Actor2D
 * and placed within a RenderWindow or ImageWindow.
 * The svtkImageMapper is a 2D mapper, which means that it displays images
 * in display coordinates. In display coordinates, one image pixel is
 * always one screen pixel.
 *
 * @sa
 * svtkMapper2D svtkActor2D
 */

#ifndef svtkImageMapper_h
#define svtkImageMapper_h

#include "svtkMapper2D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkWindow;
class svtkViewport;
class svtkActor2D;
class svtkImageData;

class SVTKRENDERINGCORE_EXPORT svtkImageMapper : public svtkMapper2D
{
public:
  svtkTypeMacro(svtkImageMapper, svtkMapper2D);
  static svtkImageMapper* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Override Modifiedtime as we have added a lookuptable
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the window value for window/level
   */
  svtkSetMacro(ColorWindow, double);
  svtkGetMacro(ColorWindow, double);
  //@}

  //@{
  /**
   * Set/Get the level value for window/level
   */
  svtkSetMacro(ColorLevel, double);
  svtkGetMacro(ColorLevel, double);
  //@}

  //@{
  /**
   * Set/Get the current slice number. The axis Z in ZSlice does not
   * necessarily have any relation to the z axis of the data on disk.
   * It is simply the axis orthogonal to the x,y, display plane.
   * GetWholeZMax and Min are convenience methods for obtaining
   * the number of slices that can be displayed. Again the number
   * of slices is in reference to the display z axis, which is not
   * necessarily the z axis on disk. (due to reformatting etc)
   */
  svtkSetMacro(ZSlice, int);
  svtkGetMacro(ZSlice, int);
  int GetWholeZMin();
  int GetWholeZMax();
  //@}

  /**
   * Draw the image to the screen.
   */
  void RenderStart(svtkViewport* viewport, svtkActor2D* actor);

  /**
   * Function called by Render to actually draw the image to to the screen
   */
  virtual void RenderData(svtkViewport*, svtkImageData*, svtkActor2D*) = 0;

  //@{
  /**
   * Methods used internally for performing the Window/Level mapping.
   */
  double GetColorShift();
  double GetColorScale();
  //@}

  // Public for templated functions. * *  Should remove this * *
  int DisplayExtent[6];

  //@{
  /**
   * Set the Input of a filter.
   */
  virtual void SetInputData(svtkImageData* input);
  svtkImageData* GetInput();
  //@}

  //@{
  /**
   * If RenderToRectangle is set (by default not), then the imagemapper
   * will render the image into the rectangle supplied by the Actor2D's
   * PositionCoordinate and Position2Coordinate
   */
  svtkSetMacro(RenderToRectangle, svtkTypeBool);
  svtkGetMacro(RenderToRectangle, svtkTypeBool);
  svtkBooleanMacro(RenderToRectangle, svtkTypeBool);
  //@}

  //@{
  /**
   * Usually, the entire image is displayed, if UseCustomExtents
   * is set (by default not), then the region supplied in the
   * CustomDisplayExtents is used in preference.
   * Note that the Custom extents are x,y only and the zslice is still
   * applied
   */
  svtkSetMacro(UseCustomExtents, svtkTypeBool);
  svtkGetMacro(UseCustomExtents, svtkTypeBool);
  svtkBooleanMacro(UseCustomExtents, svtkTypeBool);
  //@}

  //@{
  /**
   * The image extents which should be displayed with UseCustomExtents
   * Note that the Custom extents are x,y only and the zslice is still
   * applied
   */
  svtkSetVectorMacro(CustomDisplayExtents, int, 4);
  svtkGetVectorMacro(CustomDisplayExtents, int, 4);
  //@}

protected:
  svtkImageMapper();
  ~svtkImageMapper() override;

  double ColorWindow;
  double ColorLevel;

  int PositionAdjustment[2];
  int ZSlice;
  svtkTypeBool UseCustomExtents;
  int CustomDisplayExtents[4];
  svtkTypeBool RenderToRectangle;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkImageMapper(const svtkImageMapper&) = delete;
  void operator=(const svtkImageMapper&) = delete;
};

#endif
