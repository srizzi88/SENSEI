/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollisionDetection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright (c) Goodwin Lawlor All rights reserved.
  BSD 3-Clause License

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

/**
 * @class svtkCollisionDetectionFilter
 * @brief performs collision determination between two polyhedral surfaces
 *
 * svtkCollisionDetectionFilter performs collision determination between
 *  two polyhedral surfaces using two instances of svtkOBBTree. Set the
 *  polydata inputs, the tolerance and transforms or matrices. If
 *  CollisionMode is set to AllContacts, the Contacts output will be lines
 *  of contact.  If CollisionMode is FirstContact or HalfContacts then the
 *  Contacts output will be vertices.  See below for an explanation of
 *  these options.
 *
 *  This class can be used to clip one polydata surface with another,
 *  using the Contacts output as a loop set in svtkSelectPolyData
 *
 * @authors Goodwin Lawlor, Bill Lorensen
 */

//@{
/*
 * @warning
 * Currently only triangles are processed. Use svtkTriangleFilter to
 * convert any strips or polygons to triangles.
 */
//@}

//@{
/*
 * @cite
 * Goodwin Lawlor <goodwin.lawlor@ucd.ie>, University College Dublin,
 * who wrote this class.
 * Thanks to Peter C. Everett
 * <pce@world.std.com> for svtkOBBTree::IntersectWithOBBTree() in
 * particular, and all those who contributed to svtkOBBTree in general.
 * The original code was contained here: https://github.com/glawlor/svtkbioeng
 *
 */
//@}

//@{
/*
 *  @see
 *  svtkTriangleFilter, svtkSelectPolyData, svtkOBBTree
 */
//@}

#ifndef svtkCollisionDetectionFilter_h
#define svtkCollisionDetectionFilter_h

#include "svtkFieldData.h"             // For GetContactCells
#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkOBBTree;
class svtkPolyData;
class svtkPoints;
class svtkMatrix4x4;
class svtkLinearTransform;
class svtkIdTypeArray;

class SVTKFILTERSMODELING_EXPORT svtkCollisionDetectionFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type and printing.
   */
  static svtkCollisionDetectionFilter* New();
  svtkTypeMacro(svtkCollisionDetectionFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  enum CollisionModes
  {
    SVTK_ALL_CONTACTS = 0,
    SVTK_FIRST_CONTACT = 1,
    SVTK_HALF_CONTACTS = 2
  };

  //@{
  /** Set the collision mode to SVTK_ALL_CONTACTS to find all the contacting cell pairs with
   * two points per collision, or SVTK_HALF_CONTACTS to find all the contacting cell pairs
   * with one point per collision, or SVTK_FIRST_CONTACT to quickly find the first contact
   * point.
   */
  svtkSetClampMacro(CollisionMode, int, SVTK_ALL_CONTACTS, SVTK_HALF_CONTACTS);
  svtkGetMacro(CollisionMode, int);

  void SetCollisionModeToAllContacts() { this->SetCollisionMode(SVTK_ALL_CONTACTS); }
  void SetCollisionModeToFirstContact() { this->SetCollisionMode(SVTK_FIRST_CONTACT); }
  void SetCollisionModeToHalfContacts() { this->SetCollisionMode(SVTK_HALF_CONTACTS); }
  const char* GetCollisionModeAsString(void)
  {
    if (this->CollisionMode == SVTK_ALL_CONTACTS)
    {
      return "AllContacts";
    }
    else if (this->CollisionMode == SVTK_FIRST_CONTACT)
    {
      return "FirstContact";
    }
    else
    {
      return "HalfContacts";
    }
  }
  //@}

  //@{
  /**
   * Description:
   * Intersect two polygons, return x1 and x2 as the two points of intersection. If
   * CollisionMode = SVTK_ALL_CONTACTS, both contact points are found. If
   * CollisionMode = SVTK_FIRST_CONTACT or SVTK_HALF_CONTACTS, only
   * one contact point is found.
   */
  int IntersectPolygonWithPolygon(int npts, double* pts, double bounds[6], int npts2, double* pts2,
    double bounds2[6], double tol2, double x1[2], double x2[3], int CollisionMode);
  //@}

  //@{
  /**
   * Set and Get the input svtk polydata models
   */
  void SetInputData(int i, svtkPolyData* model);
  svtkPolyData* GetInputData(int i);
  //@}

  //@{
  /** Get an array of the contacting cells. This is a convenience method to access
   * the "ContactCells" field array in outputs 0 and 1. These arrays index contacting
   * cells (eg) index 50 of array 0 points to a cell (triangle) which contacts/intersects
   * a cell at index 50 of array 1. This method is equivalent to
   * GetOutput(i)->GetFieldData()->GetArray("ContactCells")
   */
  svtkIdTypeArray* GetContactCells(int i);
  //@}

  //@{
  /** Get the output with the points where the contacting cells intersect. This method is
   *  is equivalent to GetOutputPort(2)/GetOutput(2)
   */
  svtkAlgorithmOutput* GetContactsOutputPort() { return this->GetOutputPort(2); }
  svtkPolyData* GetContactsOutput() { return this->GetOutput(2); }
  //@}

  //@{
  /* Specify the transform object used to transform models. Alternatively, matrices
   * can be set instead.
`  */
  void SetTransform(int i, svtkLinearTransform* transform);
  svtkLinearTransform* GetTransform(int i) { return this->Transform[i]; }
  //@}

  //@{
  /* Specify the matrix object used to transform models.
   */
  void SetMatrix(int i, svtkMatrix4x4* matrix);
  svtkMatrix4x4* GetMatrix(int i);
  //@}

  //@{
  /* Set and Get the obb tolerance (absolute value, in world coords). Default is 0.001
   */
  svtkSetMacro(BoxTolerance, float);
  svtkGetMacro(BoxTolerance, float);
  //@}

  //@{
  /* Set and Get the cell tolerance (squared value). Default is 0.0
   */
  svtkSetMacro(CellTolerance, double);
  svtkGetMacro(CellTolerance, double);
  //@}

  //@{
  /*
   * Set and Get the the flag to visualize the contact cells. If set the contacting cells
   * will be coloured from red through to blue, with collisions first determined coloured red.
   */
  svtkSetMacro(GenerateScalars, int);
  svtkGetMacro(GenerateScalars, int);
  svtkBooleanMacro(GenerateScalars, int);
  //@}

  //@{
  /*
   * Get the number of contacting cell pairs
   */
  int GetNumberOfContacts()
  {
    return this->GetOutput(0)->GetFieldData()->GetArray("ContactCells")->GetNumberOfTuples();
  }
  //@}

  //@{Description:
  /*
   * Get the number of box tests
   */
  svtkGetMacro(NumberOfBoxTests, int);
  //@}

  //@{
  /*
   * Set and Get the number of cells in each OBB. Default is 2
   */
  svtkSetMacro(NumberOfCellsPerNode, int);
  svtkGetMacro(NumberOfCellsPerNode, int);
  //@}

  //@{
  /*
   * Set and Get the opacity of the polydata output when a collision takes place.
   * Default is 1.0
   */
  svtkSetClampMacro(Opacity, float, 0.0, 1.0);
  svtkGetMacro(Opacity, float);
  //@}

  //@{
  /*
   * Return the MTime also considering the transform.
   */
  svtkMTimeType GetMTime() override;
  //@}

protected:
  svtkCollisionDetectionFilter();
  ~svtkCollisionDetectionFilter() override;

  // Usual data generation method
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  svtkOBBTree* Tree0;
  svtkOBBTree* Tree1;

  svtkLinearTransform* Transform[2];
  svtkMatrix4x4* Matrix[2];

  int NumberOfBoxTests;

  int NumberOfCellsPerNode;

  int GenerateScalars;

  float BoxTolerance;
  float CellTolerance;
  float Opacity;

  int CollisionMode;

private:
  svtkCollisionDetectionFilter(const svtkCollisionDetectionFilter&) = delete;
  void operator=(const svtkCollisionDetectionFilter&) = delete;
};

#endif
