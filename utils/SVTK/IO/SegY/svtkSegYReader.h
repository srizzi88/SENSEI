/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSegYReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkSegYReader_h
#define svtkSegYReader_h

#include "svtkDataSetAlgorithm.h"

#include <svtkIOSegYModule.h> // For export macro

// Forward declarations
class svtkImageData;
class svtkSegYReaderInternal;

/**
 * @class svtkSegYReader
 * @brief Reads SegY data files.
 *
 * svtkSegYReader reads SegY data files. We create a svtkStructuredGrid
 * for 2.5D SegY and 3D data. If we set the StructuredGrid option to 0
 * we create a svtkImageData for 3D data. This saves memory and may
 * speed-up certain algorithms, but the position and the shape of the
 * data may not be correct. The axes for the data are: crossline,
 * inline, depth. For situations where traces are missing values of
 * zero are used to fill in the dataset.
 */
class SVTKIOSEGY_EXPORT svtkSegYReader : public svtkDataSetAlgorithm
{
public:
  static svtkSegYReader* New();
  svtkTypeMacro(svtkSegYReader, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkSegYReader();
  ~svtkSegYReader() override;

  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

  enum SVTKSegYCoordinateModes
  {
    SVTK_SEGY_SOURCE = 0, // default
    SVTK_SEGY_CDP = 1,
    SVTK_SEGY_CUSTOM = 2
  };

  //@{
  /**
   * Specify whether to use source x/y coordinates or CDP coordinates or custom
   * byte positions for data position in the SEG-Y trace header. Defaults to
   * source x/y coordinates.
   *
   * As per SEG-Y rev 2.0 specification,
   * Source XY coordinate bytes = (73, 77)
   * CDP XY coordinate bytes = (181, 185)
   */
  svtkSetClampMacro(XYCoordMode, int, SVTK_SEGY_SOURCE, SVTK_SEGY_CUSTOM);
  svtkGetMacro(XYCoordMode, int);
  void SetXYCoordModeToSource();
  void SetXYCoordModeToCDP();
  void SetXYCoordModeToCustom();
  //@}

  //@{
  /**
   * Specify X and Y byte positions for custom XYCoordinateMode.
   * By default, XCoordByte = 73, YCoordByte = 77 i.e. source xy.
   *
   * \sa SetXYCoordinatesModeToCustom()
   */
  svtkSetMacro(XCoordByte, int);
  svtkGetMacro(XCoordByte, int);
  svtkSetMacro(YCoordByte, int);
  svtkGetMacro(YCoordByte, int);
  //@}

  enum SVTKSegYVerticalCRS
  {
    SVTK_SEGY_VERTICAL_HEIGHTS = 0, // default
    SVTK_SEGY_VERTICAL_DEPTHS
  };

  //@{
  /**
   * Specify whether the vertical coordinates in the SEG-Y file are heights
   * (positive up) or depths (positive down). By default, the vertical
   * coordinates are treated as heights (i.e. positive up). This means that the
   * Z-axis of the dataset goes from 0 (surface) to -ve depth (last sample).
   * \note As per the SEG-Y rev 2.0 specification, this information is defined
   * in the Location Data Stanza of the Extended Textual Header. However, as of
   * this revision, svtkSegY2DReader does not support reading the extended
   * textual header.
   */
  svtkSetMacro(VerticalCRS, int);
  svtkGetMacro(VerticalCRS, int);
  //@}

  //@{
  /**
   * Specify if we create a svtkStructuredGrid even when the data is
   * 3D. Note this consumes more memory but it shows the precise
   * location for each point and the correct shape of the data. The
   * default value is true.  If we set this option to false we
   * create a svtkImageData for the SegY 3D dataset.
   */
  svtkSetMacro(StructuredGrid, int);
  svtkGetMacro(StructuredGrid, int);
  svtkBooleanMacro(StructuredGrid, int);
  //@}

  //@{
  /**
   * Should we force the data to be interpreted as a 2D dataset? It may be a
   * 2D sheet through 3D space. When this is turned on we ignore the cross
   * line and line values and instead build a 2D data by processing and
   * connecting the traces in order from first to last. The output will be a
   * Structrured Grid.
   */
  svtkSetMacro(Force2D, bool);
  svtkGetMacro(Force2D, bool);
  svtkBooleanMacro(Force2D, bool);
  //@}

protected:
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkSegYReaderInternal* Reader;
  char* FileName;
  bool Is3D;
  double DataOrigin[3];
  double DataSpacing[3][3];
  int DataSpacingSign[3];
  int DataExtent[6];

  int XYCoordMode;
  int StructuredGrid;

  // Custom XY coordinate byte positions
  int XCoordByte;
  int YCoordByte;

  int VerticalCRS;

  bool Force2D;

private:
  svtkSegYReader(const svtkSegYReader&) = delete;
  void operator=(const svtkSegYReader&) = delete;
};

#endif // svtkSegYReader_h
