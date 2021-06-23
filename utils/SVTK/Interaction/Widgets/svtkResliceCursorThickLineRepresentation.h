/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorThickLineRepresentation
 * @brief   represents a thick slab of the reslice cursor widget
 *
 * This class respresents a thick reslice cursor, that can be used to
 * perform interactive thick slab MPR's through data. The class internally
 * uses svtkImageSlabReslice to do its reslicing. The slab thickness is set
 * interactively from the widget. The slab resolution (ie the number of
 * blend points) is set as the minimum spacing along any dimension from
 * the dataset.
 * @sa
 * svtkImageSlabReslice svtkResliceCursorLineRepresentation svtkResliceCursorWidget
 */

#ifndef svtkResliceCursorThickLineRepresentation_h
#define svtkResliceCursorThickLineRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkResliceCursorLineRepresentation.h"

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorThickLineRepresentation
  : public svtkResliceCursorLineRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkResliceCursorThickLineRepresentation* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkResliceCursorThickLineRepresentation, svtkResliceCursorLineRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * INTERNAL - Do not use
   * Create the thick reformat class. This overrides the superclass
   * implementation and creates a svtkImageSlabReslice instead of a
   * svtkImageReslice.
   */
  void CreateDefaultResliceAlgorithm() override;

  /**
   * INTERNAL - Do not use
   * Reslice parameters which are set from svtkResliceCursorWidget based on
   * user interactions.
   */
  void SetResliceParameters(
    double outputSpacingX, double outputSpacingY, int extentX, int extentY) override;

protected:
  svtkResliceCursorThickLineRepresentation();
  ~svtkResliceCursorThickLineRepresentation() override;

private:
  svtkResliceCursorThickLineRepresentation(const svtkResliceCursorThickLineRepresentation&) = delete;
  void operator=(const svtkResliceCursorThickLineRepresentation&) = delete;
};

#endif
