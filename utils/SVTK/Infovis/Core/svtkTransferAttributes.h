/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkTransferAttributes.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkTransferAttributes
 * @brief   transfer data from a graph representation
 * to a tree representation using direct mapping or pedigree ids.
 *
 *
 * The filter requires
 * both a svtkGraph and svtkTree as input.  The tree vertices must be a
 * superset of the graph vertices.  A common example is when the graph vertices
 * correspond to the leaves of the tree, but the internal vertices of the tree
 * represent groupings of graph vertices.  The algorithm matches the vertices
 * using the array "PedigreeId".  The user may alternately set the
 * DirectMapping flag to indicate that the two structures must have directly
 * corresponding offsets (i.e. node i in the graph must correspond to node i in
 * the tree).
 *
 */

#ifndef svtkTransferAttributes_h
#define svtkTransferAttributes_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"
#include "svtkVariant.h" //For svtkVariant method arguments

class SVTKINFOVISCORE_EXPORT svtkTransferAttributes : public svtkPassInputTypeAlgorithm
{
public:
  /**
   * Create a svtkTransferAttributes object.
   * Initial values are DirectMapping = false, DefaultValue = 1,
   * SourceArrayName=0, TargetArrayName = 0,
   * SourceFieldType=svtkDataObject::FIELD_ASSOCIATION_POINTS,
   * TargetFieldType=svtkDataObject::FIELD_ASSOCIATION_POINTS
   */
  static svtkTransferAttributes* New();

  svtkTypeMacro(svtkTransferAttributes, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If on, uses direct mapping from tree to graph vertices.
   * If off, both the graph and tree must contain PedigreeId arrays
   * which are used to match graph and tree vertices.
   * Default is off.
   */
  svtkSetMacro(DirectMapping, bool);
  svtkGetMacro(DirectMapping, bool);
  svtkBooleanMacro(DirectMapping, bool);
  //@}

  //@{
  /**
   * The field name to use for storing the source array.
   */
  svtkGetStringMacro(SourceArrayName);
  svtkSetStringMacro(SourceArrayName);
  //@}

  //@{
  /**
   * The field name to use for storing the source array.
   */
  svtkGetStringMacro(TargetArrayName);
  svtkSetStringMacro(TargetArrayName);
  //@}

  //@{
  /**
   * The source field type for accessing the source array. Valid values are
   * those from enum svtkDataObject::FieldAssociations.
   */
  svtkGetMacro(SourceFieldType, int);
  svtkSetMacro(SourceFieldType, int);
  //@}

  //@{
  /**
   * The target field type for accessing the target array. Valid values are
   * those from enum svtkDataObject::FieldAssociations.
   */
  svtkGetMacro(TargetFieldType, int);
  svtkSetMacro(TargetFieldType, int);
  //@}

  //@{
  /**
   * Method to get/set the default value.
   */
  svtkVariant GetDefaultValue();
  void SetDefaultValue(svtkVariant value);
  //@}

  /**
   * Set the input type of the algorithm to svtkGraph.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkTransferAttributes();
  ~svtkTransferAttributes() override;

  bool DirectMapping;
  char* SourceArrayName;
  char* TargetArrayName;
  int SourceFieldType;
  int TargetFieldType;

  svtkVariant DefaultValue;

  /**
   * Convert the svtkGraph into svtkPolyData.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTransferAttributes(const svtkTransferAttributes&) = delete;
  void operator=(const svtkTransferAttributes&) = delete;
};

#endif
