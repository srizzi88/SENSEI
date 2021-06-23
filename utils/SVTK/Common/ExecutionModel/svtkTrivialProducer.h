/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTrivialProducer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTrivialProducer
 * @brief   Producer for stand-alone data objects.
 *
 * svtkTrivialProducer allows stand-alone data objects to be connected
 * as inputs in a pipeline.  All data objects that are connected to a
 * pipeline involving svtkAlgorithm must have a producer.  This trivial
 * producer allows data objects that are hand-constructed in a program
 * without another svtk producer to be connected.
 */

#ifndef svtkTrivialProducer_h
#define svtkTrivialProducer_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkDataObject;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkTrivialProducer : public svtkAlgorithm
{
public:
  static svtkTrivialProducer* New();
  svtkTypeMacro(svtkTrivialProducer, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Process upstream/downstream requests trivially.  The associated
   * output data object is never modified, but it is queried to
   * fulfill requests.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the data object that is "produced" by this producer.  It is
   * never really modified.
   */
  virtual void SetOutput(svtkDataObject* output);

  /**
   * The modified time of this producer is the newer of this object or
   * the assigned output.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the whole extent to use for the data this producer is producing.
   * This may be different than the extent of the output data when
   * the trivial producer is used in parallel.
   */
  svtkSetVector6Macro(WholeExtent, int);
  svtkGetVector6Macro(WholeExtent, int);
  //@}

  /**
   * This method can be used to copy meta-data from an existing data
   * object to an information object. For example, whole extent,
   * image data spacing, origin etc.
   */
  static void FillOutputDataInformation(svtkDataObject* output, svtkInformation* outInfo);

protected:
  svtkTrivialProducer();
  ~svtkTrivialProducer() override;

  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;
  svtkExecutive* CreateDefaultExecutive() override;

  // The real data object.
  svtkDataObject* Output;

  int WholeExtent[6];

  void ReportReferences(svtkGarbageCollector*) override;

private:
  svtkTrivialProducer(const svtkTrivialProducer&) = delete;
  void operator=(const svtkTrivialProducer&) = delete;
};

#endif
