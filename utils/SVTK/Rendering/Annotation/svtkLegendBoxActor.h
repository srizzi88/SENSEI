/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLegendBoxActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLegendBoxActor
 * @brief   draw symbols with text
 *
 * svtkLegendBoxActor is used to associate a symbol with a text string.
 * The user specifies a svtkPolyData to use as the symbol, and a string
 * associated with the symbol. The actor can then be placed in the scene
 * in the same way that any other svtkActor2D can be used.
 *
 * To use this class, you must define the position of the legend box by using
 * the superclasses' svtkActor2D::Position coordinate and
 * Position2 coordinate. Then define the set of symbols and text strings that
 * make up the menu box. The font attributes of the entries can be set through
 * the svtkTextProperty associated to this actor. The class will
 * scale the symbols and text to fit in the legend box defined by
 * (Position,Position2). Optional features like turning on a border line and
 * setting the spacing between the border and the symbols/text can also be
 * set.
 *
 * @sa
 * svtkXYPlotActor svtkActor2D svtkGlyphSource2D
 */

#ifndef svtkLegendBoxActor_h
#define svtkLegendBoxActor_h

#include "svtkActor2D.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

class svtkActor;
class svtkDoubleArray;
class svtkImageData;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkPolyDataMapper;
class svtkPlaneSource;
class svtkTextMapper;
class svtkTextProperty;
class svtkTexturedActor2D;
class svtkTransform;
class svtkTransformPolyDataFilter;
class svtkProperty2D;

class SVTKRENDERINGANNOTATION_EXPORT svtkLegendBoxActor : public svtkActor2D
{
public:
  svtkTypeMacro(svtkLegendBoxActor, svtkActor2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static svtkLegendBoxActor* New();

  /**
   * Specify the number of entries in the legend box.
   */
  void SetNumberOfEntries(int num);
  int GetNumberOfEntries() { return this->NumberOfEntries; }

  //@{
  /**
   * Add an entry to the legend box. You must supply a svtkPolyData to be
   * used as a symbol (it can be NULL) and a text string (which also can
   * be NULL). The svtkPolyData is assumed to be defined in the x-y plane,
   * and the text is assumed to be a single line in height. Note that when
   * this method is invoked previous entries are deleted. Also supply a text
   * string and optionally a color. (If a color is not specified, then the
   * entry color is the same as this actor's color.) (Note: use the set
   * methods when you use SetNumberOfEntries().)
   */
  void SetEntry(int i, svtkPolyData* symbol, const char* string, double color[3]);
  void SetEntry(int i, svtkImageData* symbol, const char* string, double color[3]);
  void SetEntry(
    int i, svtkPolyData* symbol, svtkImageData* icon, const char* string, double color[3]);
  //@}

  void SetEntrySymbol(int i, svtkPolyData* symbol);
  void SetEntryIcon(int i, svtkImageData* icon);
  void SetEntryString(int i, const char* string);
  void SetEntryColor(int i, double color[3]);
  void SetEntryColor(int i, double r, double g, double b);

  svtkPolyData* GetEntrySymbol(int i);
  svtkImageData* GetEntryIcon(int i);
  const char* GetEntryString(int i);
  double* GetEntryColor(int i) SVTK_SIZEHINT(3);

  //@{
  /**
   * Set/Get the text property.
   */
  virtual void SetEntryTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(EntryTextProperty, svtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the flag that controls whether a border will be drawn
   * around the legend box.
   */
  svtkSetMacro(Border, svtkTypeBool);
  svtkGetMacro(Border, svtkTypeBool);
  svtkBooleanMacro(Border, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag that controls whether the border and legend
   * placement is locked into the rectangle defined by (Position,Position2).
   * If off, then the legend box will adjust its size so that the border
   * fits nicely around the text and symbols. (The ivar is off by default.)
   * Note: the legend box is guaranteed to lie within the original border
   * definition.
   */
  svtkSetMacro(LockBorder, svtkTypeBool);
  svtkGetMacro(LockBorder, svtkTypeBool);
  svtkBooleanMacro(LockBorder, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag that controls whether a box will be drawn/filled
   * corresponding to the legend box.
   */
  svtkSetMacro(Box, svtkTypeBool);
  svtkGetMacro(Box, svtkTypeBool);
  svtkBooleanMacro(Box, svtkTypeBool);
  //@}

  /**
   * Get the box svtkProperty2D.
   */
  svtkProperty2D* GetBoxProperty() { return this->BoxActor->GetProperty(); }

  //@{
  /**
   * Set/Get the padding between the legend entries and the border. The value
   * is specified in pixels.
   */
  svtkSetClampMacro(Padding, int, 0, 50);
  svtkGetMacro(Padding, int);
  //@}

  //@{
  /**
   * Turn on/off flag to control whether the symbol's scalar data
   * is used to color the symbol. If off, the color of the
   * svtkLegendBoxActor is used.
   */
  svtkSetMacro(ScalarVisibility, svtkTypeBool);
  svtkGetMacro(ScalarVisibility, svtkTypeBool);
  svtkBooleanMacro(ScalarVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off background.
   */
  svtkSetMacro(UseBackground, svtkTypeBool);
  svtkGetMacro(UseBackground, svtkTypeBool);
  svtkBooleanMacro(UseBackground, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get background color.
   * Default is: (0.3, 0.3, 0.3).
   */
  svtkSetVector3Macro(BackgroundColor, double);
  svtkGetVector3Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * Set/Get background opacity.
   * Default is: 1.0
   */
  svtkSetClampMacro(BackgroundOpacity, double, 0.0, 1.0);
  svtkGetMacro(BackgroundOpacity, double);
  //@}

  /**
   * Shallow copy of this scaled text actor. Overloads the virtual
   * svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

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
  svtkLegendBoxActor();
  ~svtkLegendBoxActor() override;

  void InitializeEntries();

  svtkPolyData createTexturedPlane();

  svtkTypeBool Border;
  svtkTypeBool Box;
  int Padding;
  svtkTypeBool LockBorder;
  svtkTypeBool ScalarVisibility;
  double BoxOpacity;

  // Internal actors, mappers, data to represent the legend
  int NumberOfEntries;
  int Size; // allocation size
  svtkDoubleArray* Colors;
  svtkTextMapper** TextMapper;
  svtkActor2D** TextActor;

  svtkPolyData** Symbol;
  svtkTransform** Transform;
  svtkTransformPolyDataFilter** SymbolTransform;
  svtkPolyDataMapper2D** SymbolMapper;
  svtkActor2D** SymbolActor;

  svtkPlaneSource** Icon;
  svtkTransform** IconTransform;
  svtkTransformPolyDataFilter** IconTransformFilter;
  svtkPolyDataMapper2D** IconMapper;
  svtkTexturedActor2D** IconActor;
  svtkImageData** IconImage;

  svtkPolyData* BorderPolyData;
  svtkPolyDataMapper2D* BorderMapper;
  svtkActor2D* BorderActor;
  svtkPolyData* BoxPolyData;
  svtkPolyDataMapper2D* BoxMapper;
  svtkActor2D* BoxActor;
  svtkTextProperty* EntryTextProperty;

  // Background plane.
  svtkTypeBool UseBackground;
  double BackgroundOpacity;
  double BackgroundColor[3];
  svtkPlaneSource* Background;

  // May use texture.
  svtkTexturedActor2D* BackgroundActor;
  svtkPolyDataMapper2D* BackgroundMapper;

  // Used to control whether the stuff is recomputed
  int LegendEntriesVisible;
  int CachedSize[2];
  svtkTimeStamp BuildTime;

private:
  svtkLegendBoxActor(const svtkLegendBoxActor&) = delete;
  void operator=(const svtkLegendBoxActor&) = delete;
};

#endif
