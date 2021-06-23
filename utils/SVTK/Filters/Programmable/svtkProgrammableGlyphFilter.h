/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableGlyphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgrammableGlyphFilter
 * @brief   control the generation and placement of glyphs at input points
 *
 * svtkProgrammableGlyphFilter is a filter that allows you to place a glyph at
 * each input point in the dataset. In addition, the filter is programmable
 * which means the user has control over the generation of the glyph. The
 * glyphs can be controlled via the point data attributes (e.g., scalars,
 * vectors, etc.) or any other information in the input dataset.
 *
 * This is the way the filter works. You must define an input dataset which
 * at a minimum contains points with associated attribute values. Also, the
 * Source instance variable must be set which is of type svtkPolyData. Then,
 * for each point in the input, the PointId is set to the current point id,
 * and a user-defined function is called (i.e., GlyphMethod). In this method
 * you can manipulate the Source data (including changing to a different
 * Source object). After the GlyphMethod is called,
 * svtkProgrammableGlyphFilter will invoke an Update() on its Source object,
 * and then copy its data to the output of the
 * svtkProgrammableGlyphFilter. Therefore the output of this filter is of type
 * svtkPolyData.
 *
 * Another option to this filter is the way you color the glyphs. You can use
 * the scalar data from the input or the source. The instance variable
 * ColorMode controls this behavior.
 *
 * @warning
 * This filter operates on point data attributes. If you want to use cell
 * data attributes, use a filter like svtkCellCenters to generate points at
 * the centers of cells, and then use these points.
 *
 * @warning
 * Note that the data attributes (cell and point) are passed to the output of
 * this filter from the Source object. This works well as long as you are not
 * changing the class of the Source object during execution. However, if the
 * class of the Source object changes, then the potential exists that the
 * data attributes might change during execution (e.g., scalars available
 * from one source and not the next), possibly fouling up the copying of data
 * attributes to the output. In this case, you may have to manually set the
 * output's copy flags (e.g., CopyScalarsOn/Off(), CopyVectorsOn/Off(), etc.)
 * to control what's being copied.
 *
 * @sa
 * svtkGlyph3D svtkTensorGlyph svtkCellCenters
 */

#ifndef svtkProgrammableGlyphFilter_h
#define svtkProgrammableGlyphFilter_h

#define SVTK_COLOR_BY_INPUT 0
#define SVTK_COLOR_BY_SOURCE 1

#include "svtkFiltersProgrammableModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPointData;

class SVTKFILTERSPROGRAMMABLE_EXPORT svtkProgrammableGlyphFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkProgrammableGlyphFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with nullptr GlyphMethod() and no source object. The ColorMode
   * is set to color by the input.
   */
  static svtkProgrammableGlyphFilter* New();

  /**
   * Setup a connection for the source to use as the glyph.
   * Note: you can change the source during execution of this filter.
   * This is equivalent to SetInputConnection(1, output);
   */
  void SetSourceConnection(svtkAlgorithmOutput* output);

  //@{
  /**
   * Set/Get the source to use for this glyph.
   * Note that SetSourceData() does not set a pipeline connection but
   * directly uses the polydata.
   */
  void SetSourceData(svtkPolyData* source);
  svtkPolyData* GetSource();
  //@}

  /**
   * Signature definition for programmable method callbacks. Methods passed to
   * SetGlyphMethod or SetGlyphMethodArgDelete must conform to this signature.
   * The presence of this typedef is useful for reference and for external
   * analysis tools, but it cannot be used in the method signatures in these
   * header files themselves because it prevents the internal SVTK wrapper
   * generators from wrapping these methods.
   */
  typedef void (*ProgrammableMethodCallbackType)(void* arg);

  /**
   * Specify function to be called for each input point.
   */
  void SetGlyphMethod(void (*f)(void*), void* arg);

  /**
   * Set the arg delete method. This is used to free user memory that might
   * be associated with the GlyphMethod().
   */
  void SetGlyphMethodArgDelete(void (*f)(void*));

  //@{
  /**
   * Get the current point id during processing. Value only valid during the
   * Execute() method of this filter. (Meant to be called by the GlyphMethod().)
   */
  svtkGetMacro(PointId, svtkIdType);
  //@}

  //@{
  /**
   * Get the current point coordinates during processing. Value only valid during the
   * Execute() method of this filter. (Meant to be called by the GlyphMethod().)
   */
  svtkGetVector3Macro(Point, double);
  //@}

  //@{
  /**
   * Get the set of point data attributes for the input. A convenience to the
   * programmer to be used in the GlyphMethod(). Only valid during the Execute()
   * method of this filter.
   */
  svtkGetObjectMacro(PointData, svtkPointData);
  //@}

  //@{
  /**
   * Either color by the input or source scalar data.
   */
  svtkSetMacro(ColorMode, int);
  svtkGetMacro(ColorMode, int);
  void SetColorModeToColorByInput() { this->SetColorMode(SVTK_COLOR_BY_INPUT); }
  void SetColorModeToColorBySource() { this->SetColorMode(SVTK_COLOR_BY_SOURCE); }
  const char* GetColorModeAsString();
  //@}

protected:
  svtkProgrammableGlyphFilter();
  ~svtkProgrammableGlyphFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  double Point[3];   // Coordinates of point
  svtkIdType PointId; // Current point id during processing
  svtkPointData* PointData;
  int ColorMode;

  ProgrammableMethodCallbackType GlyphMethod; // Support GlyphMethod
  ProgrammableMethodCallbackType GlyphMethodArgDelete;
  void* GlyphMethodArg;

private:
  svtkProgrammableGlyphFilter(const svtkProgrammableGlyphFilter&) = delete;
  void operator=(const svtkProgrammableGlyphFilter&) = delete;
};

#endif
