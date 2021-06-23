/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProjectedTetrahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   svtkProjectedTetrahedraMapper
 * @brief   Unstructured grid volume renderer.
 *
 *
 * svtkProjectedTetrahedraMapper is an implementation of the classic
 * Projected Tetrahedra algorithm presented by Shirley and Tuchman in "A
 * Polygonal Approximation to Direct Scalar Volume Rendering" in Computer
 * Graphics, December 1990.
 *
 * @bug
 * This mapper relies highly on the implementation of the OpenGL pipeline.
 * A typical hardware driver has lots of options and some settings can
 * cause this mapper to produce artifacts.
 *
 */

#ifndef svtkProjectedTetrahedraMapper_h
#define svtkProjectedTetrahedraMapper_h

#include "svtkRenderingVolumeModule.h" // For export macro
#include "svtkUnstructuredGridVolumeMapper.h"

class svtkFloatArray;
class svtkPoints;
class svtkUnsignedCharArray;
class svtkVisibilitySort;
class svtkVolumeProperty;
class svtkRenderWindow;

class SVTKRENDERINGVOLUME_EXPORT svtkProjectedTetrahedraMapper
  : public svtkUnstructuredGridVolumeMapper
{
public:
  svtkTypeMacro(svtkProjectedTetrahedraMapper, svtkUnstructuredGridVolumeMapper);
  static svtkProjectedTetrahedraMapper* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void SetVisibilitySort(svtkVisibilitySort* sort);
  svtkGetObjectMacro(VisibilitySort, svtkVisibilitySort);

  static void MapScalarsToColors(
    svtkDataArray* colors, svtkVolumeProperty* property, svtkDataArray* scalars);
  static void TransformPoints(svtkPoints* inPoints, const float projection_mat[16],
    const float modelview_mat[16], svtkFloatArray* outPoints);

  /**
   * Return true if the rendering context provides
   * the nececessary functionality to use this class.
   */
  virtual bool IsSupported(svtkRenderWindow*) { return false; }

protected:
  svtkProjectedTetrahedraMapper();
  ~svtkProjectedTetrahedraMapper() override;

  svtkVisibilitySort* VisibilitySort;

  /**
   * The visibility sort will probably make a reference loop by holding a
   * reference to the input.
   */
  void ReportReferences(svtkGarbageCollector* collector) override;

private:
  svtkProjectedTetrahedraMapper(const svtkProjectedTetrahedraMapper&) = delete;
  void operator=(const svtkProjectedTetrahedraMapper&) = delete;
};

#endif
