/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPTSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPTSReader
 * @brief   Read ASCII PTS Files.
 *
 * svtkPTSReader reads either a text file of
 *  points. The first line is the number of points. Point information is
 *  either x y z intensity or x y z intensity r g b
 */

#ifndef svtkPTSReader_h
#define svtkPTSReader_h

#include "svtkBoundingBox.h"      // For Bounding Box Data Member
#include "svtkIOGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKIOGEOMETRY_EXPORT svtkPTSReader : public svtkPolyDataAlgorithm
{
public:
  static svtkPTSReader* New();
  svtkTypeMacro(svtkPTSReader, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name.
   */
  void SetFileName(const char* filename);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Boolean value indicates whether or not to limit points read to a specified
   * (ReadBounds) region.
   */
  svtkBooleanMacro(LimitReadToBounds, bool);
  svtkSetMacro(LimitReadToBounds, bool);
  svtkGetMacro(LimitReadToBounds, bool);
  //@}

  //@{
  /**
   * Bounds to use if LimitReadToBounds is On
   */
  svtkSetVector6Macro(ReadBounds, double);
  svtkGetVector6Macro(ReadBounds, double);
  //@}

  //@{
  /**
   * The output type defaults to float, but can instead be double.
   */
  svtkBooleanMacro(OutputDataTypeIsDouble, bool);
  svtkSetMacro(OutputDataTypeIsDouble, bool);
  svtkGetMacro(OutputDataTypeIsDouble, bool);
  //@}

  //@{
  /**
   * Boolean value indicates whether or not to limit number of points read
   * based on MaxNumbeOfPoints.
   */
  svtkBooleanMacro(LimitToMaxNumberOfPoints, bool);
  svtkSetMacro(LimitToMaxNumberOfPoints, bool);
  svtkGetMacro(LimitToMaxNumberOfPoints, bool);
  //@}

  //@{
  /**
   * The maximum number of points to load if LimitToMaxNumberOfPoints is on/true.
   * Sets a temporary onRatio.
   */
  svtkSetClampMacro(MaxNumberOfPoints, svtkIdType, 1, SVTK_INT_MAX);
  svtkGetMacro(MaxNumberOfPoints, svtkIdType);
  //@}

  //@{
  /**
   * Boolean value indicates whether or not to create cells
   * for this dataset. Otherwise only points and scalars
   * are created. Defaults to true.
   */
  svtkBooleanMacro(CreateCells, bool);
  svtkSetMacro(CreateCells, bool);
  svtkGetMacro(CreateCells, bool);
  //@}

  //@{
  /**
   * Boolean value indicates when color values are present
   * if luminance should be read in as well
   * Defaults to true.
   */
  svtkBooleanMacro(IncludeColorAndLuminance, bool);
  svtkSetMacro(IncludeColorAndLuminance, bool);
  svtkGetMacro(IncludeColorAndLuminance, bool);
  //@}

protected:
  svtkPTSReader();
  ~svtkPTSReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  bool OutputDataTypeIsDouble;

  bool LimitReadToBounds;
  double ReadBounds[6];
  svtkBoundingBox ReadBBox;
  bool LimitToMaxNumberOfPoints;
  svtkIdType MaxNumberOfPoints;
  bool CreateCells;
  bool IncludeColorAndLuminance;

private:
  svtkPTSReader(const svtkPTSReader&) = delete;
  void operator=(const svtkPTSReader&) = delete;
};

#endif
