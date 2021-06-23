/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedRepresentation.h

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
 * @class   svtkRenderedRepresentation
 *
 *
 */

#ifndef svtkRenderedRepresentation_h
#define svtkRenderedRepresentation_h

#include "svtkDataRepresentation.h"
#include "svtkSmartPointer.h"       // for SP ivars
#include "svtkUnicodeString.h"      // for string
#include "svtkViewsInfovisModule.h" // For export macro

class svtkApplyColors;
class svtkProp;
class svtkRenderView;
class svtkRenderWindow;
class svtkTextProperty;
class svtkTexture;
class svtkView;

class SVTKVIEWSINFOVIS_EXPORT svtkRenderedRepresentation : public svtkDataRepresentation
{
public:
  static svtkRenderedRepresentation* New();
  svtkTypeMacro(svtkRenderedRepresentation, svtkDataRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the label render mode.
   * svtkRenderView::QT - Use Qt-based labeler with fitted labeling
   * and unicode support. Requires SVTK_USE_QT to be on.
   * svtkRenderView::FREETYPE - Use standard freetype text rendering.
   */
  svtkSetMacro(LabelRenderMode, int);
  svtkGetMacro(LabelRenderMode, int);
  //@}

protected:
  svtkRenderedRepresentation();
  ~svtkRenderedRepresentation() override;

  //@{
  /**
   * Subclasses may call these methods to add or remove props from the representation.
   * Use these if the number of props/actors changes as the result of input connection
   * changes.
   */
  void AddPropOnNextRender(svtkProp* p);
  void RemovePropOnNextRender(svtkProp* p);
  //@}

  /**
   * Obtains the hover text for a particular prop and cell.
   * If the prop is not applicable to the representation, return an empty string.
   * Subclasses should override GetHoverTextInternal, in which the prop and cell
   * are converted to an appropriate selection using ConvertSelection().
   */
  svtkUnicodeString GetHoverText(svtkView* view, svtkProp* prop, svtkIdType cell);

  /**
   * Subclasses may override this method to generate the hover text.
   */
  virtual svtkUnicodeString GetHoverTextInternal(svtkSelection*) { return svtkUnicodeString(); }

  /**
   * The view will call this method before every render.
   * Representations may add their own pre-render logic here.
   */
  virtual void PrepareForRendering(svtkRenderView* view);

  friend class svtkRenderView;

  int LabelRenderMode;

private:
  svtkRenderedRepresentation(const svtkRenderedRepresentation&) = delete;
  void operator=(const svtkRenderedRepresentation&) = delete;

  class Internals;
  Internals* Implementation;
};

#endif
