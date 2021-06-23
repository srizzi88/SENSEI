/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBalloonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBalloonRepresentation
 * @brief   represent the svtkBalloonWidget
 *
 * The svtkBalloonRepresentation is used to represent the svtkBalloonWidget.
 * This representation is defined by two items: a text string and an image.
 * At least one of these two items must be defined, but it is allowable to
 * specify both, or just an image or just text. If both the text and image
 * are specified, then methods are available for positioning the text and
 * image with respect to each other.
 *
 * The balloon representation consists of three parts: text, a rectangular
 * frame behind the text, and an image placed next to the frame and sized
 * to match the frame.
 *
 * The size of the balloon is ultimately controlled by the text properties
 * (i.e., font size). This representation uses a layout policy as follows.
 *
 * If there is just text and no image, then the text properties and padding
 * are used to control the size of the balloon.
 *
 * If there is just an image and no text, then the ImageSize[2] member is
 * used to control the image size. (The image will fit into this rectangle,
 * but will not necessarily fill the whole rectangle, i.e., the image is not
 * stretched).
 *
 * If there is text and an image, the following approach ia used. First,
 * based on the font size and other related properties (e.g., padding),
 * determine the size of the frame. Second, depending on the layout of the
 * image and text frame, control the size of the neighboring image (since the
 * frame and image share a common edge). However, if this results in an image
 * that is smaller than ImageSize[2], then the image size will be set to
 * ImageSize[2] and the frame will be adjusted accordingly. The text is
 * always placed in the center of the frame if the frame is resized.
 *
 * @sa
 * svtkBalloonWidget
 */

#ifndef svtkBalloonRepresentation_h
#define svtkBalloonRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkWidgetRepresentation.h"

class svtkTextMapper;
class svtkTextActor;
class svtkTextProperty;
class svtkPoints;
class svtkCellArray;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkActor2D;
class svtkProperty2D;
class svtkImageData;
class svtkTexture;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTexturedActor2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkBalloonRepresentation : public svtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkBalloonRepresentation* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkBalloonRepresentation, svtkWidgetRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify/retrieve the image to display in the balloon.
   */
  virtual void SetBalloonImage(svtkImageData* img);
  svtkGetObjectMacro(BalloonImage, svtkImageData);
  //@}

  //@{
  /**
   * Specify/retrieve the text to display in the balloon.
   */
  svtkGetStringMacro(BalloonText);
  svtkSetStringMacro(BalloonText);
  //@}

  //@{
  /**
   * Specify the minimum size for the image. Note that this is a bounding
   * rectangle, the image will fit inside of it. However, if the balloon
   * consists of text plus an image, then the image may be bigger than
   * ImageSize[2] to fit into the balloon frame.
   */
  svtkSetVector2Macro(ImageSize, int);
  svtkGetVector2Macro(ImageSize, int);
  //@}

  //@{
  /**
   * Set/get the text property (relevant only if text is shown).
   */
  virtual void SetTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/get the frame property (relevant only if text is shown).
   * The frame lies behind the text.
   */
  virtual void SetFrameProperty(svtkProperty2D* p);
  svtkGetObjectMacro(FrameProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Set/get the image property (relevant only if an image is shown).
   */
  virtual void SetImageProperty(svtkProperty2D* p);
  svtkGetObjectMacro(ImageProperty, svtkProperty2D);
  //@}

  enum
  {
    ImageLeft = 0,
    ImageRight,
    ImageBottom,
    ImageTop
  };

  //@{
  /**
   * Specify the layout of the image and text within the balloon. Note that
   * there are reduncies in these methods, for example
   * SetBalloonLayoutToImageLeft() results in the same effect as
   * SetBalloonLayoutToTextRight(). If only text is specified, or only an
   * image is specified, then it doesn't matter how the layout is specified.
   */
  svtkSetMacro(BalloonLayout, int);
  svtkGetMacro(BalloonLayout, int);
  void SetBalloonLayoutToImageLeft() { this->SetBalloonLayout(ImageLeft); }
  void SetBalloonLayoutToImageRight() { this->SetBalloonLayout(ImageRight); }
  void SetBalloonLayoutToImageBottom() { this->SetBalloonLayout(ImageBottom); }
  void SetBalloonLayoutToImageTop() { this->SetBalloonLayout(ImageTop); }
  void SetBalloonLayoutToTextLeft() { this->SetBalloonLayout(ImageRight); }
  void SetBalloonLayoutToTextRight() { this->SetBalloonLayout(ImageLeft); }
  void SetBalloonLayoutToTextTop() { this->SetBalloonLayout(ImageBottom); }
  void SetBalloonLayoutToTextBottom() { this->SetBalloonLayout(ImageTop); }
  //@}

  //@{
  /**
   * Set/Get the offset from the mouse pointer from which to place the
   * balloon. The representation will try and honor this offset unless there
   * is a collision with the side of the renderer, in which case the balloon
   * will be repositioned to lie within the rendering window.
   */
  svtkSetVector2Macro(Offset, int);
  svtkGetVector2Macro(Offset, int);
  //@}

  //@{
  /**
   * Set/Get the padding (in pixels) that is used between the text and the
   * frame.
   */
  svtkSetClampMacro(Padding, int, 0, 100);
  svtkGetMacro(Padding, int);
  //@}

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void StartWidgetInteraction(double e[2]) override;
  void EndWidgetInteraction(double e[2]) override;
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  //@}

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * State is either outside, or inside (on the text portion of the image).
   */
  enum _InteractionState
  {
    Outside = 0,
    OnText,
    OnImage
  };

protected:
  svtkBalloonRepresentation();
  ~svtkBalloonRepresentation() override;

  // The balloon text and image
  char* BalloonText;
  svtkImageData* BalloonImage;

  // The layout of the balloon
  int BalloonLayout;

  // Controlling placement
  int Padding;
  int Offset[2];
  int ImageSize[2];

  // Represent the text
  svtkTextMapper* TextMapper;
  svtkActor2D* TextActor;
  svtkTextProperty* TextProperty;

  // Represent the image
  svtkTexture* Texture;
  svtkPolyData* TexturePolyData;
  svtkPoints* TexturePoints;
  svtkPolyDataMapper2D* TextureMapper;
  svtkTexturedActor2D* TextureActor;
  svtkProperty2D* ImageProperty;

  // The frame
  svtkPoints* FramePoints;
  svtkCellArray* FramePolygon;
  svtkPolyData* FramePolyData;
  svtkPolyDataMapper2D* FrameMapper;
  svtkActor2D* FrameActor;
  svtkProperty2D* FrameProperty;

  // Internal variable controlling rendering process
  int TextVisible;
  int ImageVisible;

  // Helper methods
  void AdjustImageSize(double imageSize[2]);
  void ScaleImage(double imageSize[2], double scale);

private:
  svtkBalloonRepresentation(const svtkBalloonRepresentation&) = delete;
  void operator=(const svtkBalloonRepresentation&) = delete;
};

#endif
