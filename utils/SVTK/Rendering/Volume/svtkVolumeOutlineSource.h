/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeOutlineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumeOutlineSource
 * @brief   outline of volume cropping region
 *
 * svtkVolumeOutlineSource generates a wireframe outline that corresponds
 * to the cropping region of a svtkVolumeMapper.  It requires a
 * svtkVolumeMapper as input.  The GenerateFaces option turns on the
 * solid faces of the outline, and the GenerateScalars option generates
 * color scalars.  When GenerateScalars is on, it is possible to set
 * an "ActivePlaneId" value in the range [0..6] to highlight one of the
 * six cropping planes.
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkVolumeOutlineSource_h
#define svtkVolumeOutlineSource_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkVolumeMapper;

class SVTKRENDERINGVOLUME_EXPORT svtkVolumeOutlineSource : public svtkPolyDataAlgorithm
{
public:
  static svtkVolumeOutlineSource* New();
  svtkTypeMacro(svtkVolumeOutlineSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the mapper that has the cropping region that the outline will
   * be generated for.  The mapper must have an input, because the
   * bounds of the data must be computed in order to generate the
   * outline.
   */
  virtual void SetVolumeMapper(svtkVolumeMapper* mapper);
  svtkVolumeMapper* GetVolumeMapper() { return this->VolumeMapper; }
  //@}

  //@{
  /**
   * Set whether to generate color scalars for the output.  By default,
   * the output has no scalars and the color must be set in the
   * property of the actor.
   */
  svtkSetMacro(GenerateScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateScalars, svtkTypeBool);
  svtkGetMacro(GenerateScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to generate an outline wherever an input face was
   * cut by a plane.  This is on by default.
   */
  svtkSetMacro(GenerateOutline, svtkTypeBool);
  svtkBooleanMacro(GenerateOutline, svtkTypeBool);
  svtkGetMacro(GenerateOutline, svtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to generate polygonal faces for the output.  By default,
   * only lines are generated.  The faces will form a closed, watertight
   * surface.
   */
  svtkSetMacro(GenerateFaces, svtkTypeBool);
  svtkBooleanMacro(GenerateFaces, svtkTypeBool);
  svtkGetMacro(GenerateFaces, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the color of the outline.  This has no effect unless GenerateScalars
   * is On.  The default color is red.
   */
  svtkSetVector3Macro(Color, double);
  svtkGetVector3Macro(Color, double);
  //@}

  //@{
  /**
   * Set the active plane, e.g. to display which plane is currently being
   * modified by an interaction.  Set this to -1 if there is no active plane.
   * The default value is -1.
   */
  svtkSetMacro(ActivePlaneId, int);
  svtkGetMacro(ActivePlaneId, int);
  //@}

  //@{
  /**
   * Set the color of the active cropping plane.  This has no effect unless
   * GenerateScalars is On and ActivePlaneId is non-negative.  The default
   * color is yellow.
   */
  svtkSetVector3Macro(ActivePlaneColor, double);
  svtkGetVector3Macro(ActivePlaneColor, double);
  //@}

protected:
  svtkVolumeOutlineSource();
  ~svtkVolumeOutlineSource() override;

  svtkVolumeMapper* VolumeMapper;
  svtkTypeBool GenerateScalars;
  svtkTypeBool GenerateOutline;
  svtkTypeBool GenerateFaces;
  int ActivePlaneId;
  double Color[3];
  double ActivePlaneColor[3];

  int Cropping;
  int CroppingRegionFlags;
  double Bounds[6];
  double CroppingRegionPlanes[6];

  static int ComputeCubePlanes(double planes[3][4], double croppingPlanes[6], double bounds[6]);

  static void GeneratePolys(svtkCellArray* polys, svtkUnsignedCharArray* scalars,
    unsigned char colors[2][3], int activePlane, int flags, int tolPtId[3][4]);

  static void GenerateLines(svtkCellArray* lines, svtkUnsignedCharArray* scalars,
    unsigned char colors[2][3], int activePlane, int flags, int tolPtId[3][4]);

  static void GeneratePoints(
    svtkPoints* points, svtkCellArray* lines, svtkCellArray* polys, double planes[3][4], double tol);

  static void NudgeCropPlanesToBounds(int tolPtId[3][4], double planes[3][4], double tol);

  static void CreateColorValues(unsigned char colors[2][3], double color1[3], double color2[3]);

  int ComputePipelineMTime(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, int requestFromOutputPort, svtkMTimeType* mtime) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkVolumeOutlineSource(const svtkVolumeOutlineSource&) = delete;
  void operator=(const svtkVolumeOutlineSource&) = delete;
};

#endif
