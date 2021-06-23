/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSetToLabelHierarchy.h

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
 * @class   svtkPointSetToLabelHierarchy
 * @brief   build a label hierarchy for a graph or point set.
 *
 *
 *
 * Every point in the input svtkPoints object is taken to be an
 * anchor point for a label. Statistics on the input points
 * are used to subdivide an octree referencing the points
 * until the points each octree node contains have a variance
 * close to the node size and a limited population (< 100).
 */

#ifndef svtkPointSetToLabelHierarchy_h
#define svtkPointSetToLabelHierarchy_h

#include "svtkLabelHierarchyAlgorithm.h"
#include "svtkRenderingLabelModule.h" // For export macro

class svtkTextProperty;

class SVTKRENDERINGLABEL_EXPORT svtkPointSetToLabelHierarchy : public svtkLabelHierarchyAlgorithm
{
public:
  static svtkPointSetToLabelHierarchy* New();
  svtkTypeMacro(svtkPointSetToLabelHierarchy, svtkLabelHierarchyAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the "ideal" number of labels to associate with each node in the output hierarchy.
   */
  svtkSetMacro(TargetLabelCount, int);
  svtkGetMacro(TargetLabelCount, int);
  //@}

  //@{
  /**
   * Set/get the maximum tree depth in the output hierarchy.
   */
  svtkSetMacro(MaximumDepth, int);
  svtkGetMacro(MaximumDepth, int);
  //@}

  //@{
  /**
   * Whether to use unicode strings.
   */
  svtkSetMacro(UseUnicodeStrings, bool);
  svtkGetMacro(UseUnicodeStrings, bool);
  svtkBooleanMacro(UseUnicodeStrings, bool);
  //@}

  //@{
  /**
   * Set/get the label array name.
   */
  virtual void SetLabelArrayName(const char* name);
  virtual const char* GetLabelArrayName();
  //@}

  //@{
  /**
   * Set/get the priority array name.
   */
  virtual void SetSizeArrayName(const char* name);
  virtual const char* GetSizeArrayName();
  //@}

  //@{
  /**
   * Set/get the priority array name.
   */
  virtual void SetPriorityArrayName(const char* name);
  virtual const char* GetPriorityArrayName();
  //@}

  //@{
  /**
   * Set/get the icon index array name.
   */
  virtual void SetIconIndexArrayName(const char* name);
  virtual const char* GetIconIndexArrayName();
  //@}

  //@{
  /**
   * Set/get the text orientation array name.
   */
  virtual void SetOrientationArrayName(const char* name);
  virtual const char* GetOrientationArrayName();
  //@}

  //@{
  /**
   * Set/get the maximum text width (in world coordinates) array name.
   */
  virtual void SetBoundedSizeArrayName(const char* name);
  virtual const char* GetBoundedSizeArrayName();
  //@}

  //@{
  /**
   * Set/get the text property assigned to the hierarchy.
   */
  virtual void SetTextProperty(svtkTextProperty* tprop);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

protected:
  svtkPointSetToLabelHierarchy();
  ~svtkPointSetToLabelHierarchy() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int TargetLabelCount;
  int MaximumDepth;
  bool UseUnicodeStrings;
  svtkTextProperty* TextProperty;

private:
  svtkPointSetToLabelHierarchy(const svtkPointSetToLabelHierarchy&) = delete;
  void operator=(const svtkPointSetToLabelHierarchy&) = delete;
};

#endif // svtkPointSetToLabelHierarchy_h
