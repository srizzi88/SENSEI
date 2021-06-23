/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextProperty
 * @brief   represent text properties.
 *
 * svtkTextProperty is an object that represents text properties.
 * The primary properties that can be set are color, opacity, font size,
 * font family horizontal and vertical justification, bold/italic/shadow
 * styles.
 * @sa
 * svtkTextMapper svtkTextActor svtkLegendBoxActor svtkCaptionActor2D
 */

#ifndef svtkTextProperty_h
#define svtkTextProperty_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkTextProperty : public svtkObject
{
public:
  svtkTypeMacro(svtkTextProperty, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new text property with font size 12, bold off, italic off,
   * and Arial font.
   */
  static svtkTextProperty* New();

  //@{
  /**
   * Set the color of the text.
   */
  svtkSetVector3Macro(Color, double);
  svtkGetVector3Macro(Color, double);
  //@}

  //@{
  /**
   * Set/Get the text's opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  svtkSetClampMacro(Opacity, double, 0., 1.);
  svtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * The background color.
   */
  svtkSetVector3Macro(BackgroundColor, double);
  svtkGetVector3Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * The background opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  svtkSetClampMacro(BackgroundOpacity, double, 0., 1.);
  svtkGetMacro(BackgroundOpacity, double);
  //@}

  //@{
  /**
   * The frame color.
   */
  svtkSetVector3Macro(FrameColor, double);
  svtkGetVector3Macro(FrameColor, double);
  //@}

  //@{
  /**
   * Enable/disable text frame.
   */
  svtkSetMacro(Frame, svtkTypeBool);
  svtkGetMacro(Frame, svtkTypeBool);
  svtkBooleanMacro(Frame, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the width of the frame. The width is expressed in pixels.
   * The default is 1 pixel.
   */
  svtkSetClampMacro(FrameWidth, int, 0, SVTK_INT_MAX);
  svtkGetMacro(FrameWidth, int);
  //@}

  //@{
  /**
   * Set/Get the font family. Supports legacy three font family system.
   * If the symbolic constant SVTK_FONT_FILE is returned by GetFontFamily(), the
   * string returned by GetFontFile() must be an absolute filepath
   * to a local FreeType compatible font.
   */
  svtkGetStringMacro(FontFamilyAsString);
  svtkSetStringMacro(FontFamilyAsString);
  void SetFontFamily(int t);
  int GetFontFamily();
  int GetFontFamilyMinValue() { return SVTK_ARIAL; }
  void SetFontFamilyToArial();
  void SetFontFamilyToCourier();
  void SetFontFamilyToTimes();
  static int GetFontFamilyFromString(const char* f);
  static const char* GetFontFamilyAsString(int f);
  //@}

  //@{
  /**
   * The absolute filepath to a local file containing a freetype-readable font
   * if GetFontFamily() return SVTK_FONT_FILE. The result is undefined for other
   * values of GetFontFamily().
   */
  svtkGetStringMacro(FontFile);
  svtkSetStringMacro(FontFile);
  //@}

  //@{
  /**
   * Set/Get the font size (in points).
   */
  svtkSetClampMacro(FontSize, int, 0, SVTK_INT_MAX);
  svtkGetMacro(FontSize, int);
  //@}

  //@{
  /**
   * Enable/disable text bolding.
   */
  svtkSetMacro(Bold, svtkTypeBool);
  svtkGetMacro(Bold, svtkTypeBool);
  svtkBooleanMacro(Bold, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable text italic.
   */
  svtkSetMacro(Italic, svtkTypeBool);
  svtkGetMacro(Italic, svtkTypeBool);
  svtkBooleanMacro(Italic, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable text shadow.
   */
  svtkSetMacro(Shadow, svtkTypeBool);
  svtkGetMacro(Shadow, svtkTypeBool);
  svtkBooleanMacro(Shadow, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the shadow offset, i.e. the distance from the text to
   * its shadow, in the same unit as FontSize.
   */
  svtkSetVector2Macro(ShadowOffset, int);
  svtkGetVectorMacro(ShadowOffset, int, 2);
  //@}

  /**
   * Get the shadow color. It is computed from the Color ivar
   */
  void GetShadowColor(double color[3]);

  //@{
  /**
   * Set/Get the horizontal justification to left (default), centered,
   * or right.
   */
  svtkSetClampMacro(Justification, int, SVTK_TEXT_LEFT, SVTK_TEXT_RIGHT);
  svtkGetMacro(Justification, int);
  void SetJustificationToLeft() { this->SetJustification(SVTK_TEXT_LEFT); }
  void SetJustificationToCentered() { this->SetJustification(SVTK_TEXT_CENTERED); }
  void SetJustificationToRight() { this->SetJustification(SVTK_TEXT_RIGHT); }
  const char* GetJustificationAsString();
  //@}

  //@{
  /**
   * Set/Get the vertical justification to bottom (default), middle,
   * or top.
   */
  svtkSetClampMacro(VerticalJustification, int, SVTK_TEXT_BOTTOM, SVTK_TEXT_TOP);
  svtkGetMacro(VerticalJustification, int);
  void SetVerticalJustificationToBottom() { this->SetVerticalJustification(SVTK_TEXT_BOTTOM); }
  void SetVerticalJustificationToCentered() { this->SetVerticalJustification(SVTK_TEXT_CENTERED); }
  void SetVerticalJustificationToTop() { this->SetVerticalJustification(SVTK_TEXT_TOP); }
  const char* GetVerticalJustificationAsString();
  //@}

  //@{
  /**
   * If this property is on, text is aligned to drawn pixels not to font metrix.
   * If the text does not include descents, the bounding box will not extend below
   * the baseline. This option can be used to get centered labels. It does not
   * work well if the string changes as the string position will move around.
   */
  svtkSetMacro(UseTightBoundingBox, svtkTypeBool);
  svtkGetMacro(UseTightBoundingBox, svtkTypeBool);
  svtkBooleanMacro(UseTightBoundingBox, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the text's orientation (in degrees).
   */
  svtkSetMacro(Orientation, double);
  svtkGetMacro(Orientation, double);
  //@}

  //@{
  /**
   * Set/Get the (extra) spacing between lines,
   * expressed as a text height multiplication factor.
   */
  svtkSetMacro(LineSpacing, double);
  svtkGetMacro(LineSpacing, double);
  //@}

  //@{
  /**
   * Set/Get the vertical offset (measured in pixels).
   */
  svtkSetMacro(LineOffset, double);
  svtkGetMacro(LineOffset, double);
  //@}

  /**
   * Shallow copy of a text property.
   */
  void ShallowCopy(svtkTextProperty* tprop);

protected:
  svtkTextProperty();
  ~svtkTextProperty() override;

  double Color[3];
  double Opacity;
  double BackgroundColor[3];
  double BackgroundOpacity;
  svtkTypeBool Frame;
  double FrameColor[3];
  int FrameWidth;
  char* FontFamilyAsString;
  char* FontFile;
  int FontSize;
  svtkTypeBool Bold;
  svtkTypeBool Italic;
  svtkTypeBool Shadow;
  int ShadowOffset[2];
  int Justification;
  int VerticalJustification;
  svtkTypeBool UseTightBoundingBox;
  double Orientation;
  double LineOffset;
  double LineSpacing;

private:
  svtkTextProperty(const svtkTextProperty&) = delete;
  void operator=(const svtkTextProperty&) = delete;
};

inline const char* svtkTextProperty::GetFontFamilyAsString(int f)
{
  if (f == SVTK_ARIAL)
  {
    return "Arial";
  }
  else if (f == SVTK_COURIER)
  {
    return "Courier";
  }
  else if (f == SVTK_TIMES)
  {
    return "Times";
  }
  else if (f == SVTK_FONT_FILE)
  {
    return "File";
  }
  return "Unknown";
}

inline void svtkTextProperty::SetFontFamily(int t)
{
  this->SetFontFamilyAsString(this->GetFontFamilyAsString(t));
}

inline void svtkTextProperty::SetFontFamilyToArial()
{
  this->SetFontFamily(SVTK_ARIAL);
}

inline void svtkTextProperty::SetFontFamilyToCourier()
{
  this->SetFontFamily(SVTK_COURIER);
}

inline void svtkTextProperty::SetFontFamilyToTimes()
{
  this->SetFontFamily(SVTK_TIMES);
}

inline int svtkTextProperty::GetFontFamilyFromString(const char* f)
{
  if (strcmp(f, GetFontFamilyAsString(SVTK_ARIAL)) == 0)
  {
    return SVTK_ARIAL;
  }
  else if (strcmp(f, GetFontFamilyAsString(SVTK_COURIER)) == 0)
  {
    return SVTK_COURIER;
  }
  else if (strcmp(f, GetFontFamilyAsString(SVTK_TIMES)) == 0)
  {
    return SVTK_TIMES;
  }
  else if (strcmp(f, GetFontFamilyAsString(SVTK_FONT_FILE)) == 0)
  {
    return SVTK_FONT_FILE;
  }
  return SVTK_UNKNOWN_FONT;
}

inline int svtkTextProperty::GetFontFamily()
{
  return GetFontFamilyFromString(this->FontFamilyAsString);
}

inline const char* svtkTextProperty::GetJustificationAsString(void)
{
  if (this->Justification == SVTK_TEXT_LEFT)
  {
    return "Left";
  }
  else if (this->Justification == SVTK_TEXT_CENTERED)
  {
    return "Centered";
  }
  else if (this->Justification == SVTK_TEXT_RIGHT)
  {
    return "Right";
  }
  return "Unknown";
}

inline const char* svtkTextProperty::GetVerticalJustificationAsString(void)
{
  if (this->VerticalJustification == SVTK_TEXT_BOTTOM)
  {
    return "Bottom";
  }
  else if (this->VerticalJustification == SVTK_TEXT_CENTERED)
  {
    return "Centered";
  }
  else if (this->VerticalJustification == SVTK_TEXT_TOP)
  {
    return "Top";
  }
  return "Unknown";
}

#endif
