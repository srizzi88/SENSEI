/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridHomogeneousRayIntegrator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   svtkUnstructuredGridHomogeneousRayIntegrator
 * @brief   performs piecewise constant ray integration.
 *
 *
 *
 * svtkUnstructuredGridHomogeneousRayIntegrator performs homogeneous ray
 * integration.  This is a good method to use when volume rendering scalars
 * that are defined on cells.
 *
 */

#ifndef svtkUnstructuredGridHomogeneousRayIntegrator_h
#define svtkUnstructuredGridHomogeneousRayIntegrator_h

#include "svtkRenderingVolumeModule.h" // For export macro
#include "svtkUnstructuredGridVolumeRayIntegrator.h"

class svtkVolumeProperty;

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridHomogeneousRayIntegrator
  : public svtkUnstructuredGridVolumeRayIntegrator
{
public:
  svtkTypeMacro(svtkUnstructuredGridHomogeneousRayIntegrator, svtkUnstructuredGridVolumeRayIntegrator);
  static svtkUnstructuredGridHomogeneousRayIntegrator* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Initialize(svtkVolume* volume, svtkDataArray* scalars) override;

  void Integrate(svtkDoubleArray* intersectionLengths, svtkDataArray* nearIntersections,
    svtkDataArray* farIntersections, float color[4]) override;

  //@{
  /**
   * For quick lookup, the transfer function is sampled into a table.
   * This parameter sets how big of a table to use.  By default, 1024
   * entries are used.
   */
  svtkSetMacro(TransferFunctionTableSize, int);
  svtkGetMacro(TransferFunctionTableSize, int);
  //@}

protected:
  svtkUnstructuredGridHomogeneousRayIntegrator();
  ~svtkUnstructuredGridHomogeneousRayIntegrator() override;

  svtkVolume* Volume;
  svtkVolumeProperty* Property;

  int NumComponents;
  float** ColorTable;
  float** AttenuationTable;
  double* TableShift;
  double* TableScale;
  svtkTimeStamp TablesBuilt;

  int UseAverageColor;
  int TransferFunctionTableSize;

  virtual void GetTransferFunctionTables(svtkDataArray* scalars);

private:
  svtkUnstructuredGridHomogeneousRayIntegrator(
    const svtkUnstructuredGridHomogeneousRayIntegrator&) = delete;
  void operator=(const svtkUnstructuredGridHomogeneousRayIntegrator&) = delete;
};

#endif // svtkUnstructuredGridHomogeneousRayIntegrator_h
