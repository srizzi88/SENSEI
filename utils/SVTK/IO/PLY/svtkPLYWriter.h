/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLYWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPLYWriter
 * @brief   write Stanford PLY file format
 *
 * svtkPLYWriter writes polygonal data in Stanford University PLY format
 * (see http://graphics.stanford.edu/data/3Dscanrep/). The data can be
 * written in either binary (little or big endian) or ASCII representation.
 * As for PointData and CellData, svtkPLYWriter cannot handle normals or
 * vectors. It only handles RGB PointData and CellData. You need to set the
 * name of the array (using SetName for the array and SetArrayName for the
 * writer). If the array is not a svtkUnsignedCharArray with 3 or 4 components,
 * you need to specify a svtkLookupTable to map the scalars to RGB.
 *
 * To enable saving out alpha (opacity) values, you must enable alpha using
 * `svtkPLYWriter::SetEnableAlpha()`.
 *
 * @warning
 * PLY does not handle big endian versus little endian correctly.
 *
 * @sa
 * svtkPLYReader
 */

#ifndef svtkPLYWriter_h
#define svtkPLYWriter_h

#include "svtkIOPLYModule.h"  // For export macro
#include "svtkSmartPointer.h" // For protected ivars
#include "svtkWriter.h"

#include <string> // For string parameter

class svtkDataSetAttributes;
class svtkPolyData;
class svtkScalarsToColors;
class svtkStringArray;
class svtkUnsignedCharArray;

#define SVTK_LITTLE_ENDIAN 0
#define SVTK_BIG_ENDIAN 1

#define SVTK_COLOR_MODE_DEFAULT 0
#define SVTK_COLOR_MODE_UNIFORM_CELL_COLOR 1
#define SVTK_COLOR_MODE_UNIFORM_POINT_COLOR 2
#define SVTK_COLOR_MODE_UNIFORM_COLOR 3
#define SVTK_COLOR_MODE_OFF 4

#define SVTK_TEXTURECOORDS_UV 0
#define SVTK_TEXTURECOORDS_TEXTUREUV 1

class SVTKIOPLY_EXPORT svtkPLYWriter : public svtkWriter
{
public:
  static svtkPLYWriter* New();
  svtkTypeMacro(svtkPLYWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If the file type is binary, then the user can specify which
   * byte order to use (little versus big endian).
   */
  svtkSetClampMacro(DataByteOrder, int, SVTK_LITTLE_ENDIAN, SVTK_BIG_ENDIAN);
  svtkGetMacro(DataByteOrder, int);
  void SetDataByteOrderToBigEndian() { this->SetDataByteOrder(SVTK_BIG_ENDIAN); }
  void SetDataByteOrderToLittleEndian() { this->SetDataByteOrder(SVTK_LITTLE_ENDIAN); }
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   * Note that writing to an output stream would be more flexible (enabling
   * other kind of streams) and possibly more efficient because we don't need
   * to write the whole stream to a string. However a stream interface
   * does not translate well to python and the string interface satisfies
   * our current needs. So we leave the stream interface for future work.
   */
  svtkSetMacro(WriteToOutputString, bool);
  svtkGetMacro(WriteToOutputString, bool);
  svtkBooleanMacro(WriteToOutputString, bool);
  const std::string& GetOutputString() const { return this->OutputString; }
  //@}

  //@{
  /**
   * These methods enable the user to control how to add color into the PLY
   * output file. The default behavior is as follows. The user provides the
   * name of an array and a component number. If the type of the array is
   * three components, unsigned char, then the data is written as three
   * separate "red", "green" and "blue" properties. If the type of the array is
   * four components, unsigned char, then the data is written as three separate
   * "red", "green" and "blue" properties, dropping the "alpha". If the type is not
   * unsigned char, and a lookup table is provided, then the array/component
   * are mapped through the table to generate three separate "red", "green"
   * and "blue" properties in the PLY file. The user can also set the
   * ColorMode to specify a uniform color for the whole part (on a vertex
   * colors, face colors, or both. (Note: vertex colors or cell colors may be
   * written, depending on where the named array is found. If points and
   * cells have the arrays with the same name, then both colors will be
   * written.)
   */
  svtkSetMacro(ColorMode, int);
  svtkGetMacro(ColorMode, int);
  void SetColorModeToDefault() { this->SetColorMode(SVTK_COLOR_MODE_DEFAULT); }
  void SetColorModeToUniformCellColor() { this->SetColorMode(SVTK_COLOR_MODE_UNIFORM_CELL_COLOR); }
  void SetColorModeToUniformPointColor() { this->SetColorMode(SVTK_COLOR_MODE_UNIFORM_POINT_COLOR); }
  void SetColorModeToUniformColor() // both cells and points are colored
  {
    this->SetColorMode(SVTK_COLOR_MODE_UNIFORM_COLOR);
  }
  void SetColorModeToOff() // No color information is written
  {
    this->SetColorMode(SVTK_COLOR_MODE_OFF);
  }
  //@}

  //@{
  /**
   * Enable alpha output. Default is off, i.e. only color values will be saved
   * based on ColorMode.
   */
  svtkSetMacro(EnableAlpha, bool);
  svtkGetMacro(EnableAlpha, bool);
  svtkBooleanMacro(EnableAlpha, bool);
  //@}

  //@{
  /**
   * Specify the array name to use to color the data.
   */
  svtkSetStringMacro(ArrayName);
  svtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Specify the array component to use to color the data.
   */
  svtkSetClampMacro(Component, int, 0, SVTK_INT_MAX);
  svtkGetMacro(Component, int);
  //@}

  //@{
  /**
   * A lookup table can be specified in order to convert data arrays to
   * RGBA colors.
   */
  virtual void SetLookupTable(svtkScalarsToColors*);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Set the color to use when using a uniform color (either point or cells,
   * or both). The color is specified as a triplet of three unsigned chars
   * between (0,255). This only takes effect when the ColorMode is set to
   * uniform point, uniform cell, or uniform color.
   */
  svtkSetVector3Macro(Color, unsigned char);
  svtkGetVector3Macro(Color, unsigned char);
  //@}

  //@{
  /** Set the alpha to use when using a uniform color (effect point or cells, or
   *  both) and EnableAlpha is ON.
   */
  svtkSetMacro(Alpha, unsigned char);
  svtkGetMacro(Alpha, unsigned char);
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  svtkPolyData* GetInput();
  svtkPolyData* GetInput(int port);
  //@}

  //@{
  /**
   * Specify file name of svtk polygon data file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify file type (ASCII or BINARY) for svtk data file.
   */
  svtkSetClampMacro(FileType, int, SVTK_ASCII, SVTK_BINARY);
  svtkGetMacro(FileType, int);
  void SetFileTypeToASCII() { this->SetFileType(SVTK_ASCII); }
  void SetFileTypeToBinary() { this->SetFileType(SVTK_BINARY); }
  //@}

  //@{
  /**
   * Choose the name used for the texture coordinates.
   * (u, v) or (texture_u, texture_v)
   */
  svtkSetClampMacro(TextureCoordinatesName, int, SVTK_TEXTURECOORDS_UV, SVTK_TEXTURECOORDS_TEXTUREUV);
  svtkGetMacro(TextureCoordinatesName, int);
  void SetTextureCoordinatesNameToUV() { this->SetTextureCoordinatesName(SVTK_TEXTURECOORDS_UV); }
  void SetTextureCoordinatesNameToTextureUV()
  {
    this->SetTextureCoordinatesName(SVTK_TEXTURECOORDS_TEXTUREUV);
  }
  //@}

  /**
   * Add a comment in the header part.
   */
  void AddComment(const std::string& comment);

protected:
  svtkPLYWriter();
  ~svtkPLYWriter() override;

  void WriteData() override;
  svtkSmartPointer<svtkUnsignedCharArray> GetColors(svtkIdType num, svtkDataSetAttributes* dsa);
  const float* GetTextureCoordinates(svtkIdType num, svtkDataSetAttributes* dsa);

  int DataByteOrder;
  char* ArrayName;
  int Component;
  int ColorMode;
  svtkScalarsToColors* LookupTable;
  unsigned char Color[3];

  bool EnableAlpha;
  unsigned char Alpha;

  char* FileName;

  int FileType;
  int TextureCoordinatesName;

  svtkSmartPointer<svtkStringArray> HeaderComments;

  // Whether this object is writing to a string or a file.
  // Default is 0: write to file.
  bool WriteToOutputString;

  // The output string.
  std::string OutputString;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPLYWriter(const svtkPLYWriter&) = delete;
  void operator=(const svtkPLYWriter&) = delete;
};

#endif
