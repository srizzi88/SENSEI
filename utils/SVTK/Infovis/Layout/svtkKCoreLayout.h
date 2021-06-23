/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkKCoreLayout.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkKCoreLayout
 * @brief   Produces a layout for a graph labeled with K-Core
 *                        information.
 *
 *
 * svtkKCoreLayout creates coordinates for each vertex that can be used to
 * perform a layout for a k-core view.
 * Prerequisite:  Vertices must have an attribute array containing their
 *                k-core number.  This layout is based on the algorithm
 *                described in the paper: "k-core decomposition: a tool
 *                for the visualization of large scale networks", by
 *                Ignacio Alvarez-Hamelin et. al.
 *
 *                This graph algorithm appends either polar coordinates or cartesian coordinates
 *                as vertex attributes to the graph giving the position of the vertex in a layout.
 *                Input graphs must have the k-core number assigned to each vertex (default
 *                attribute array storing kcore numbers is "kcore").
 *
 *                Epsilon - this factor is used to adjust the amount vertices are 'pulled' out of
 *                          their default ring radius based on the number of neighbors in higher
 *                          cores.  Default=0.2
 *                UnitRadius - Tweaks the base unit radius value.  Default=1.0
 *
 *                Still TODO: Still need to work on the connected-components within each shell and
 *                            associated layout issues with that.
 *
 * Input port 0: graph
 *
 * @par Thanks:
 * Thanks to William McLendon from Sandia National Laboratories for providing this
 * implementation.
 */

#ifndef svtkKCoreLayout_h
#define svtkKCoreLayout_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class SVTKINFOVISLAYOUT_EXPORT svtkKCoreLayout : public svtkGraphAlgorithm
{
public:
  static svtkKCoreLayout* New();
  svtkTypeMacro(svtkKCoreLayout, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /// Convenience function provided for setting the graph input.
  void SetGraphConnection(svtkAlgorithmOutput*);

  svtkKCoreLayout();
  ~svtkKCoreLayout() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  //@{
  /**
   * Set the name of the vertex attribute array storing k-core labels.
   * Default: kcore
   */
  svtkSetStringMacro(KCoreLabelArrayName);
  //@}

  //@{
  /**
   * Output polar coordinates for vertices if True.  Default column names are
   * coord_radius, coord_angle.
   * Default: False
   */
  svtkGetMacro(Polar, bool);
  svtkSetMacro(Polar, bool);
  svtkBooleanMacro(Polar, bool);
  //@}

  //@{
  /**
   * Set whether or not to convert output to cartesian coordinates.  If false, coordinates
   * will be returned in polar coordinates (radius, angle).
   * Default: True
   */
  svtkGetMacro(Cartesian, bool);
  svtkSetMacro(Cartesian, bool);
  svtkBooleanMacro(Cartesian, bool);
  //@}

  //@{
  /**
   * Polar coordinates array name for radius values.
   * This is only used if OutputCartesianCoordinates is False.
   * Default: coord_radius
   */
  svtkSetStringMacro(PolarCoordsRadiusArrayName);
  svtkGetStringMacro(PolarCoordsRadiusArrayName);
  //@}

  //@{
  /**
   * Polar coordinates array name for angle values in radians.
   * This is only used if OutputCartesianCoordinates is False.
   * Default: coord_angle
   */
  svtkSetStringMacro(PolarCoordsAngleArrayName);
  svtkGetStringMacro(PolarCoordsAngleArrayName);
  //@}

  //@{
  /**
   * Cartesian coordinates array name for the X coordinates.
   * This is only used if OutputCartesianCoordinates is True.
   * Default: coord_x
   */
  svtkSetStringMacro(CartesianCoordsXArrayName);
  svtkGetStringMacro(CartesianCoordsXArrayName);
  //@}

  //@{
  /**
   * Cartesian coordinates array name for the Y coordinates.
   * This is only used if OutputCartesianCoordinates is True.
   * Default: coord_y
   */
  svtkSetStringMacro(CartesianCoordsYArrayName);
  svtkGetStringMacro(CartesianCoordsYArrayName);
  //@}

  //@{
  /**
   * Epsilon value used in the algorithm.
   * Default = 0.2
   */
  svtkSetMacro(Epsilon, float);
  svtkGetMacro(Epsilon, float);
  //@}

  //@{
  /**
   * Unit Radius value used in the algorithm.
   * Default = 1.0
   */
  svtkSetMacro(UnitRadius, float);
  svtkGetMacro(UnitRadius, float);
  //@}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  char* KCoreLabelArrayName;

  char* PolarCoordsRadiusArrayName;
  char* PolarCoordsAngleArrayName;

  char* CartesianCoordsXArrayName;
  char* CartesianCoordsYArrayName;

  bool Cartesian;
  bool Polar;

  float Epsilon;
  float UnitRadius;

private:
  svtkKCoreLayout(const svtkKCoreLayout&) = delete;
  void operator=(const svtkKCoreLayout&) = delete;
};

#endif
