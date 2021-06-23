/*=========================================================================

  Program: Visualization Toolkit
  Module: svtkProStarReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProStarReader
 * @brief   Reads geometry in proSTAR (STARCD) file format.
 *
 * svtkProStarReader creates an unstructured grid dataset.
 * It reads .cel/.vrt files stored in proSTAR (STARCD) ASCII format.
 *
 * @par Thanks:
 * Reader written by Mark Olesen
 *
 */

#ifndef svtkProStarReader_h
#define svtkProStarReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKIOGEOMETRY_EXPORT svtkProStarReader : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkProStarReader* New();
  svtkTypeMacro(svtkProStarReader, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the file name prefix of the cel/vrt files to read.
   * The reader will try to open FileName.cel and FileName.vrt files.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * The proSTAR files are often in millimeters.
   * Specify an alternative scaling factor.
   */
  svtkSetClampMacro(ScaleFactor, double, 0, SVTK_DOUBLE_MAX);
  svtkGetMacro(ScaleFactor, double);
  //@}

  /**
   * The type of material represented by the cell
   */
  enum cellType
  {
    starcdFluidType = 1,
    starcdSolidType = 2,
    starcdBaffleType = 3,
    starcdShellType = 4,
    starcdLineType = 5,
    starcdPointType = 6
  };

  /**
   * The primitive cell shape
   */
  enum shapeType
  {
    starcdPoint = 1,
    starcdLine = 2,
    starcdShell = 3,
    starcdHex = 11,
    starcdPrism = 12,
    starcdTet = 13,
    starcdPyr = 14,
    starcdPoly = 255
  };

protected:
  svtkProStarReader();
  ~svtkProStarReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * The name of the file to be read.  If it has a .cel, .vrt, or .inp
   * extension it will be truncated and later appended when reading
   * the appropriate files.  Otherwise those extensions will be appended
   * to FileName when opening the files.
   */
  char* FileName;

  /**
   * The coordinates are multiplied by ScaleFactor when setting them.
   * The default value is 1.
   */
  double ScaleFactor;

private:
  //
  // Internal Classes/Structures
  //
  struct idMapping;

  FILE* OpenFile(const char* ext);

  bool ReadVrtFile(svtkUnstructuredGrid* output, idMapping& pointMapping);
  bool ReadCelFile(svtkUnstructuredGrid* output, const idMapping& pointMapping);

  svtkProStarReader(const svtkProStarReader&) = delete;
  void operator=(const svtkProStarReader&) = delete;
};
#endif
