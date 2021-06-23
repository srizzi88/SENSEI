/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDEMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDEMReader
 * @brief   read a digital elevation model (DEM) file
 *
 * svtkDEMReader reads digital elevation files and creates image data.
 * Digital elevation files are produced by the
 * <A HREF="http://www.usgs.gov">US Geological Survey</A>.
 * A complete description of the DEM file is located at the USGS site.
 * The reader reads the entire dem file and create a svtkImageData that
 * contains a single scalar component that is the elevation in meters.
 * The spacing is also expressed in meters. A number of get methods
 * provide access to fields on the header.
 */

#ifndef svtkDEMReader_h
#define svtkDEMReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKIOIMAGE_EXPORT svtkDEMReader : public svtkImageAlgorithm
{
public:
  static svtkDEMReader* New();
  svtkTypeMacro(svtkDEMReader, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of Digital Elevation Model (DEM) file
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  enum
  {
    REFERENCE_SEA_LEVEL = 0,
    REFERENCE_ELEVATION_BOUNDS
  };

  //@{
  /**
   * Specify the elevation origin to use. By default, the elevation origin
   * is equal to ElevationBounds[0]. A more convenient origin is to use sea
   * level (i.e., a value of 0.0).
   */
  svtkSetClampMacro(ElevationReference, int, REFERENCE_SEA_LEVEL, REFERENCE_ELEVATION_BOUNDS);
  svtkGetMacro(ElevationReference, int);
  void SetElevationReferenceToSeaLevel() { this->SetElevationReference(REFERENCE_SEA_LEVEL); }
  void SetElevationReferenceToElevationBounds()
  {
    this->SetElevationReference(REFERENCE_ELEVATION_BOUNDS);
  }
  const char* GetElevationReferenceAsString(void);
  //@}

  //@{
  /**
   * An ASCII description of the map
   */
  svtkGetStringMacro(MapLabel);
  //@}

  //@{
  /**
   * Code 1=DEM-1, 2=DEM_2, ...
   */
  svtkGetMacro(DEMLevel, int);
  //@}

  //@{
  /**
   * Code 1=regular, 2=random, reserved for future use
   */
  svtkGetMacro(ElevationPattern, int);
  //@}

  //@{
  /**
   * Ground planimetric reference system
   */
  svtkGetMacro(GroundSystem, int);
  //@}

  //@{
  /**
   * Zone in ground planimetric reference system
   */
  svtkGetMacro(GroundZone, int);
  //@}

  //@{
  /**
   * Map Projection parameters. All are zero.
   */
  svtkGetVectorMacro(ProjectionParameters, float, 15);
  //@}

  //@{
  /**
   * Defining unit of measure for ground planimetric coordinates throughout
   * the file. 0 = radians, 1 = feet, 2 = meters, 3 = arc-seconds.
   */
  svtkGetMacro(PlaneUnitOfMeasure, int);
  //@}

  //@{
  /**
   * Defining unit of measure for elevation coordinates throughout
   * the file. 1 = feet, 2 = meters
   */
  svtkGetMacro(ElevationUnitOfMeasure, int);
  //@}

  //@{
  /**
   * Number of sides in the polygon which defines the coverage of
   * the DEM file. Set to 4.
   */
  svtkGetMacro(PolygonSize, int);
  //@}

  //@{
  /**
   * Minimum and maximum elevation for the DEM. The units in the file
   * are in ElevationUnitOfMeasure. This class converts them to meters.
   */
  svtkGetVectorMacro(ElevationBounds, float, 2);
  //@}

  //@{
  /**
   * Counterclockwise angle (in radians) from the primary axis of the planimetric
   * reference to the primary axis of the DEM local reference system.
   * IGNORED BY THIS IMPLEMENTATION.
   */
  svtkGetMacro(LocalRotation, float);
  //@}

  //@{
  /**
   * Accuracy code for elevations. 0=unknown accuracy
   */
  svtkGetMacro(AccuracyCode, int);
  //@}

  //@{
  /**
   * DEM spatial resolution for x,y,z. Values are expressed in units of resolution.
   * Since elevations are read as integers, this permits fractional elevations.
   */
  svtkGetVectorMacro(SpatialResolution, float, 3);
  //@}

  //@{
  /**
   * The number of rows and columns in the DEM.
   */
  svtkGetVectorMacro(ProfileDimension, int, 2);
  //@}

  /**
   * Reads the DEM Type A record to compute the extent, origin and
   * spacing of the image data. The number of scalar components is set
   * to 1 and the output scalar type is SVTK_FLOAT.
   */
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkDEMReader();
  ~svtkDEMReader() override;

  svtkTimeStamp ReadHeaderTime;
  int NumberOfColumns;
  int NumberOfRows;
  int WholeExtent[6];
  char* FileName;
  char MapLabel[145];
  int DEMLevel;
  int ElevationPattern;
  int GroundSystem;
  int GroundZone;
  float ProjectionParameters[15];
  int PlaneUnitOfMeasure;
  int ElevationUnitOfMeasure;
  int PolygonSize;
  float GroundCoords[4][2];
  float ElevationBounds[2];
  float LocalRotation;
  int AccuracyCode;
  float SpatialResolution[3];
  int ProfileDimension[2];
  int ProfileSeekOffset;
  int ElevationReference;

  void ComputeExtentOriginAndSpacing(int extent[6], double origin[6], double spacing[6]);
  int ReadTypeARecord();
  int ReadProfiles(svtkImageData* data);
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkDEMReader(const svtkDEMReader&) = delete;
  void operator=(const svtkDEMReader&) = delete;
};

#endif
