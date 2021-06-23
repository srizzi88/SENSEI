/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCaptionActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCaptionActor2D
 * @brief   draw text label associated with a point
 *
 * svtkCaptionActor2D is a hybrid 2D/3D actor that is used to associate text
 * with a point (the AttachmentPoint) in the scene. The caption can be
 * drawn with a rectangular border and a leader connecting
 * the caption to the attachment point. Optionally, the leader can be
 * glyphed at its endpoint to create arrow heads or other indicators.
 *
 * To use the caption actor, you normally specify the Position and Position2
 * coordinates (these are inherited from the svtkActor2D superclass). (Note
 * that Position2 can be set using svtkActor2D's SetWidth() and SetHeight()
 * methods.)  Position and Position2 define the size of the caption, and a
 * third point, the AttachmentPoint, defines a point that the caption is
 * associated with.  You must also define the caption text,
 * whether you want a border around the caption, and whether you want a
 * leader from the caption to the attachment point. The font attributes of
 * the text can be set through the svtkTextProperty associated to this actor.
 * You also indicate whether you want
 * the leader to be 2D or 3D. (2D leaders are always drawn over the
 * underlying geometry. 3D leaders may be occluded by the geometry.) The
 * leader may also be terminated by an optional glyph (e.g., arrow).
 *
 * The trickiest part about using this class is setting Position, Position2,
 * and AttachmentPoint correctly. These instance variables are
 * svtkCoordinates, and can be set up in various ways. In default usage, the
 * AttachmentPoint is defined in the world coordinate system, Position is the
 * lower-left corner of the caption and relative to AttachmentPoint (defined
 * in display coordaintes, i.e., pixels), and Position2 is relative to
 * Position and is the upper-right corner (also in display
 * coordinates). However, the user has full control over the coordinates, and
 * can do things like place the caption in a fixed position in the renderer,
 * with the leader moving with the AttachmentPoint.
 *
 * @sa
 * svtkLegendBoxActor svtkTextMapper svtkTextActor svtkTextProperty
 * svtkCoordinate
 */

#ifndef svtkCaptionActor2D_h
#define svtkCaptionActor2D_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkActor;
class svtkAlgorithmOutput;
class svtkAppendPolyData;
class svtkCaptionActor2DConnection;
class svtkGlyph2D;
class svtkGlyph3D;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkPolyDataMapper;
class svtkTextActor;
class svtkTextMapper;
class svtkTextProperty;

class SVTKRENDERINGANNOTATION_EXPORT svtkCaptionActor2D : public svtkActor2D
{
public:
  svtkTypeMacro(svtkCaptionActor2D, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkCaptionActor2D* New();

  //@{
  /**
   * Define the text to be placed in the caption. The text can be multiple
   * lines (separated by "\n").
   */
  virtual void SetCaption(const char* caption);
  virtual char* GetCaption();
  //@}

  //@{
  /**
   * Set/Get the attachment point for the caption. By default, the attachment
   * point is defined in world coordinates, but this can be changed using
   * svtkCoordinate methods.
   */
  svtkWorldCoordinateMacro(AttachmentPoint);
  //@}

  //@{
  /**
   * Enable/disable the placement of a border around the text.
   */
  svtkSetMacro(Border, svtkTypeBool);
  svtkGetMacro(Border, svtkTypeBool);
  svtkBooleanMacro(Border, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable drawing a "line" from the caption to the
   * attachment point.
   */
  svtkSetMacro(Leader, svtkTypeBool);
  svtkGetMacro(Leader, svtkTypeBool);
  svtkBooleanMacro(Leader, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether the leader is 2D (no hidden line) or 3D (z-buffered).
   */
  svtkSetMacro(ThreeDimensionalLeader, svtkTypeBool);
  svtkGetMacro(ThreeDimensionalLeader, svtkTypeBool);
  svtkBooleanMacro(ThreeDimensionalLeader, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify a glyph to be used as the leader "head". This could be something
   * like an arrow or sphere. If not specified, no glyph is drawn. Note that
   * the glyph is assumed to be aligned along the x-axis and is rotated about
   * the origin. SetLeaderGlyphData() directly uses the polydata without
   * setting a pipeline connection. SetLeaderGlyphConnection() sets up a
   * pipeline connection and causes an update to the input during render.
   */
  virtual void SetLeaderGlyphData(svtkPolyData*);
  virtual void SetLeaderGlyphConnection(svtkAlgorithmOutput*);
  virtual svtkPolyData* GetLeaderGlyph();
  //@}

  //@{
  /**
   * Specify the relative size of the leader head. This is expressed as a
   * fraction of the size (diagonal length) of the renderer. The leader
   * head is automatically scaled so that window resize, zooming or other
   * camera motion results in proportional changes in size to the leader
   * glyph.
   */
  svtkSetClampMacro(LeaderGlyphSize, double, 0.0, 0.1);
  svtkGetMacro(LeaderGlyphSize, double);
  //@}

  //@{
  /**
   * Specify the maximum size of the leader head (if any) in pixels. This
   * is used in conjunction with LeaderGlyphSize to cap the maximum size of
   * the LeaderGlyph.
   */
  svtkSetClampMacro(MaximumLeaderGlyphSize, int, 1, 1000);
  svtkGetMacro(MaximumLeaderGlyphSize, int);
  //@}

  //@{
  /**
   * Set/Get the padding between the caption and the border. The value
   * is specified in pixels.
   */
  svtkSetClampMacro(Padding, int, 0, 50);
  svtkGetMacro(Padding, int);
  //@}

  //@{
  /**
   * Get the text actor used by the caption. This is useful if you want to control
   * justification and other characteristics of the text actor.
   */
  svtkGetObjectMacro(TextActor, svtkTextActor);
  //@}

  //@{
  /**
   * Set/Get the text property.
   */
  virtual void SetCaptionTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(CaptionTextProperty, svtkTextProperty);
  //@}

  /**
   * Shallow copy of this scaled text actor. Overloads the virtual
   * svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  //@{
  /**
   * Enable/disable whether to attach the arrow only to the edge,
   * NOT the vertices of the caption border.
   */
  svtkSetMacro(AttachEdgeOnly, svtkTypeBool);
  svtkGetMacro(AttachEdgeOnly, svtkTypeBool);
  svtkBooleanMacro(AttachEdgeOnly, svtkTypeBool);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
   * Draw the legend box to the screen.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override { return 0; }
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

protected:
  svtkCaptionActor2D();
  ~svtkCaptionActor2D() override;

  svtkCoordinate* AttachmentPointCoordinate;

  svtkTypeBool Border;
  svtkTypeBool Leader;
  svtkTypeBool ThreeDimensionalLeader;
  double LeaderGlyphSize;
  int MaximumLeaderGlyphSize;

  int Padding;
  svtkTypeBool AttachEdgeOnly;

private:
  svtkTextActor* TextActor;
  svtkTextProperty* CaptionTextProperty;

  svtkPolyData* BorderPolyData;
  svtkPolyDataMapper2D* BorderMapper;
  svtkActor2D* BorderActor;

  svtkPolyData* HeadPolyData;       // single attachment point for glyphing
  svtkGlyph3D* HeadGlyph;           // for 3D leader
  svtkPolyData* LeaderPolyData;     // line represents the leader
  svtkAppendPolyData* AppendLeader; // append head and leader

  // for 2D leader
  svtkCoordinate* MapperCoordinate2D;
  svtkPolyDataMapper2D* LeaderMapper2D;
  svtkActor2D* LeaderActor2D;

  // for 3D leader
  svtkPolyDataMapper* LeaderMapper3D;
  svtkActor* LeaderActor3D;

  svtkCaptionActor2DConnection* LeaderGlyphConnectionHolder;

private:
  svtkCaptionActor2D(const svtkCaptionActor2D&) = delete;
  void operator=(const svtkCaptionActor2D&) = delete;
};

#endif
