/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridAlgorithm
 * @brief   Superclass for algorithms that produce
 * a hyper tree grid as output
 *
 *
 * svtkHyperTreeGridAlgorithm is a base class for hyper tree grid algorithms.
 * This class defaults with one input port and one output port; it must be
 * modified by the concrete derived class if a different behavior is sought.
 * In addition, this class provides a FillOutputPortInfo() method that, by
 * default, specifies that the output is a data object; this
 * must also be modified in concrete subclasses if needed.
 *
 * @par Thanks:
 * This test was written by Philippe Pebay and Charles Law, Kitware 2012
 * This test was rewritten by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridAlgorithm_h
#define svtkHyperTreeGridAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkBitArray;
class svtkDataSetAttributes;
class svtkHyperTreeGrid;
class svtkPolyData;
class svtkUnstructuredGrid;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkHyperTreeGridAlgorithm : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkHyperTreeGridAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkDataObject* GetOutput();
  svtkDataObject* GetOutput(int);
  virtual void SetOutput(svtkDataObject*);
  //@}

  //@{
  /**
   * Get the output as a hyper tree grid.
   */
  svtkHyperTreeGrid* GetHyperTreeGridOutput();
  svtkHyperTreeGrid* GetHyperTreeGridOutput(int);
  //@}

  //@{
  /**
   * Get the output as a polygonal dataset.
   */
  svtkPolyData* GetPolyDataOutput();
  svtkPolyData* GetPolyDataOutput(int);
  //@}

  //@{
  /**
   * Get the output as an unstructured grid.
   */
  svtkUnstructuredGrid* GetUnstructuredGridOutput();
  svtkUnstructuredGrid* GetUnstructuredGridOutput(int);
  //@}

  /**
   * See svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(int, svtkDataObject*);
  //@}

protected:
  svtkHyperTreeGridAlgorithm();
  ~svtkHyperTreeGridAlgorithm() override;

  /**
   * see svtkAlgorithm for details
   */
  int RequestDataObject(
    svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector);

  // convenience method
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  /**
   * Main routine to process individual trees in the grid
   * This is pure virtual method to be implemented by concrete algorithms
   */
  virtual int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) = 0;

  //@{
  /**
   * Define default input and output port types
   */
  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;
  //@}

  //@{
  /**
   * Reference to input and output data
   */
  svtkDataSetAttributes* InData;
  svtkDataSetAttributes* OutData;
  //@}

  //@{
  /**
   * JB Si a vrai, l'objet output aura le meme type que le type d'objet en entree input.
   */
  bool AppropriateOutput;
  //@}

private:
  svtkHyperTreeGridAlgorithm(const svtkHyperTreeGridAlgorithm&) = delete;
  void operator=(const svtkHyperTreeGridAlgorithm&) = delete;
};

#endif
