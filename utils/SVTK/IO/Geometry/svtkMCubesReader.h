/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMCubesReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMCubesReader
 * @brief   read binary marching cubes file
 *
 * svtkMCubesReader is a source object that reads binary marching cubes
 * files. (Marching cubes is an isosurfacing technique that generates
 * many triangles.) The binary format is supported by W. Lorensen's
 * marching cubes program (and the svtkSliceCubes object). The format
 * repeats point coordinates, so this object will merge the points
 * with a svtkLocator object. You can choose to supply the svtkLocator
 * or use the default.
 *
 * @warning
 * Binary files assumed written in sun/hp/sgi (i.e., Big Endian) form.
 *
 * @warning
 * Because points are merged when read, degenerate triangles may be removed.
 * Thus the number of triangles read may be fewer than the number of triangles
 * actually created.
 *
 * @warning
 * The point merging does not take into account that the same point may have
 * different normals. For example, running svtkPolyDataNormals after
 * svtkContourFilter may split triangles because of the FeatureAngle
 * ivar. Subsequent reading with svtkMCubesReader will merge the points and
 * use the first point's normal. For the most part, this is undesirable.
 *
 * @warning
 * Normals are generated from the gradient of the data scalar values. Hence
 * the normals may on occasion point in a direction inconsistent with the
 * ordering of the triangle vertices. If this happens, the resulting surface
 * may be "black".  Reverse the sense of the FlipNormals boolean flag to
 * correct this.
 *
 * @sa
 * svtkContourFilter svtkMarchingCubes svtkSliceCubes svtkLocator
 */

#ifndef svtkMCubesReader_h
#define svtkMCubesReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define SVTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class svtkIncrementalPointLocator;

class SVTKIOGEOMETRY_EXPORT svtkMCubesReader : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkMCubesReader, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with FlipNormals turned off and Normals set to true.
   */
  static svtkMCubesReader* New();

  //@{
  /**
   * Specify file name of marching cubes file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set / get the file name of the marching cubes limits file.
   */
  svtkSetStringMacro(LimitsFileName);
  svtkGetStringMacro(LimitsFileName);
  //@}

  //@{
  /**
   * Specify a header size if one exists. The header is skipped and not used at this time.
   */
  svtkSetClampMacro(HeaderSize, int, 0, SVTK_INT_MAX);
  svtkGetMacro(HeaderSize, int);
  //@}

  //@{
  /**
   * Specify whether to flip normals in opposite direction. Flipping ONLY
   * changes the direction of the normal vector. Contrast this with flipping
   * in svtkPolyDataNormals which flips both the normal and the cell point
   * order.
   */
  svtkSetMacro(FlipNormals, svtkTypeBool);
  svtkGetMacro(FlipNormals, svtkTypeBool);
  svtkBooleanMacro(FlipNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether to read normals.
   */
  svtkSetMacro(Normals, svtkTypeBool);
  svtkGetMacro(Normals, svtkTypeBool);
  svtkBooleanMacro(Normals, svtkTypeBool);
  //@}

  //@{
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian otherwise
   * SetDataByteOrderToBigEndian.
   */
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char* GetDataByteOrderAsString();
  //@}

  //@{
  /**
   * Turn on/off byte swapping.
   */
  svtkSetMacro(SwapBytes, svtkTypeBool);
  svtkGetMacro(SwapBytes, svtkTypeBool);
  svtkBooleanMacro(SwapBytes, svtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkMCubesReader();
  ~svtkMCubesReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  char* LimitsFileName;
  svtkIncrementalPointLocator* Locator;
  svtkTypeBool SwapBytes;
  int HeaderSize;
  svtkTypeBool FlipNormals;
  svtkTypeBool Normals;

private:
  svtkMCubesReader(const svtkMCubesReader&) = delete;
  void operator=(const svtkMCubesReader&) = delete;
};

#endif
