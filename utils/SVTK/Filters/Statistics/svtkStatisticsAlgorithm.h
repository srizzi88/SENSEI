/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkStatisticsAlgorithm.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkStatisticsAlgorithm
 * @brief   Base class for statistics algorithms
 *
 *
 * All statistics algorithms can conceptually be operated with several operations:
 * * Learn: given an input data set, calculate a minimal statistical model (e.g.,
 *   sums, raw moments, joint probabilities).
 * * Derive: given an input minimal statistical model, derive the full model
 *   (e.g., descriptive statistics, quantiles, correlations, conditional
 *    probabilities).
 *   NB: It may be, or not be, a problem that a full model was not derived. For
 *   instance, when doing parallel calculations, one only wants to derive the full
 *   model after all partial calculations have completed. On the other hand, one
 *   can also directly provide a full model, that was previously calculated or
 *   guessed, and not derive a new one.
 * * Assess: given an input data set, input statistics, and some form of
 *   threshold, assess a subset of the data set.
 * * Test: perform at least one statistical test.
 * Therefore, a svtkStatisticsAlgorithm has the following ports
 * * 3 optional input ports:
 *   * Data (svtkTable)
 *   * Parameters to the learn operation (svtkTable)
 *   * Input model (svtkMultiBlockDataSet)
 * * 3 output ports:
 *   * Data (input annotated with assessments when the Assess operation is ON).
 *   * Output model (identical to the input model when Learn operation is OFF).
 *   * Output of statistical tests. Some engines do not offer such tests yet, in
 *     which case this output will always be empty even when the Test operation is ON.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef svtkStatisticsAlgorithm_h
#define svtkStatisticsAlgorithm_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class svtkDataObjectCollection;
class svtkMultiBlockDataSet;
class svtkStdString;
class svtkStringArray;
class svtkVariant;
class svtkVariantArray;
class svtkDoubleArray;
class svtkStatisticsAlgorithmPrivate;

class SVTKFILTERSSTATISTICS_EXPORT svtkStatisticsAlgorithm : public svtkTableAlgorithm
{
public:
  svtkTypeMacro(svtkStatisticsAlgorithm, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * enumeration values to specify input port types
   */
  enum InputPorts
  {
    INPUT_DATA = 0,       //!< Port 0 is for learn data
    LEARN_PARAMETERS = 1, //!< Port 1 is for learn parameters (initial guesses, etc.)
    INPUT_MODEL = 2       //!< Port 2 is for a priori models
  };

  /**
   * enumeration values to specify output port types
   */
  enum OutputIndices
  {
    OUTPUT_DATA = 0,  //!< Output 0 mirrors the input data, plus optional assessment columns
    OUTPUT_MODEL = 1, //!< Output 1 contains any generated model
    OUTPUT_TEST = 2   //!< Output 2 contains result of statistical test(s)
  };

  /**
   * A convenience method for setting learn input parameters (if one is expected or allowed).
   * It is equivalent to calling SetInputConnection( 1, params );
   */
  virtual void SetLearnOptionParameterConnection(svtkAlgorithmOutput* params)
  {
    this->SetInputConnection(svtkStatisticsAlgorithm::LEARN_PARAMETERS, params);
  }

  /**
   * A convenience method for setting learn input parameters (if one is expected or allowed).
   * It is equivalent to calling SetInputData( 1, params );
   */
  virtual void SetLearnOptionParameters(svtkDataObject* params)
  {
    this->SetInputData(svtkStatisticsAlgorithm::LEARN_PARAMETERS, params);
  }

  /**
   * A convenience method for setting the input model connection (if one is expected or allowed).
   * It is equivalent to calling SetInputConnection( 2, model );
   */
  virtual void SetInputModelConnection(svtkAlgorithmOutput* model)
  {
    this->SetInputConnection(svtkStatisticsAlgorithm::INPUT_MODEL, model);
  }

  /**
   * A convenience method for setting the input model (if one is expected or allowed).
   * It is equivalent to calling SetInputData( 2, model );
   */
  virtual void SetInputModel(svtkDataObject* model)
  {
    this->SetInputData(svtkStatisticsAlgorithm::INPUT_MODEL, model);
  }

  //@{
  /**
   * Set/Get the Learn operation.
   */
  svtkSetMacro(LearnOption, bool);
  svtkGetMacro(LearnOption, bool);
  //@}

  //@{
  /**
   * Set/Get the Derive operation.
   */
  svtkSetMacro(DeriveOption, bool);
  svtkGetMacro(DeriveOption, bool);
  //@}

  //@{
  /**
   * Set/Get the Assess operation.
   */
  svtkSetMacro(AssessOption, bool);
  svtkGetMacro(AssessOption, bool);
  //@}

  //@{
  /**
   * Set/Get the Test operation.
   */
  svtkSetMacro(TestOption, bool);
  svtkGetMacro(TestOption, bool);
  //@}

  //@{
  /**
   * Set/Get the number of tables in the primary model.
   */
  svtkSetMacro(NumberOfPrimaryTables, svtkIdType);
  svtkGetMacro(NumberOfPrimaryTables, svtkIdType);
  //@}

  //@{
  /**
   * Set/get assessment names.
   */
  virtual void SetAssessNames(svtkStringArray*);
  svtkGetObjectMacro(AssessNames, svtkStringArray);
  //@}

  //@{
  /**
   * A base class for a functor that assesses data.
   */
  class AssessFunctor
  {
  public:
    virtual void operator()(svtkDoubleArray*, svtkIdType) = 0;
    virtual ~AssessFunctor() {}
  };
  //@}

  /**
   * Add or remove a column from the current analysis request.
   * Once all the column status values are set, call RequestSelectedColumns()
   * before selecting another set of columns for a different analysis request.
   * The way that columns selections are used varies from algorithm to algorithm.

   * Note: the set of selected columns is maintained in svtkStatisticsAlgorithmPrivate::Buffer
   * until RequestSelectedColumns() is called, at which point the set is appended
   * to svtkStatisticsAlgorithmPrivate::Requests.
   * If there are any columns in svtkStatisticsAlgorithmPrivate::Buffer at the time
   * RequestData() is called, RequestSelectedColumns() will be called and the
   * selection added to the list of requests.
   */
  virtual void SetColumnStatus(const char* namCol, int status);

  /**
   * Set the status of each and every column in the current request to OFF (0).
   */
  virtual void ResetAllColumnStates();

  /**
   * Use the current column status values to produce a new request for statistics
   * to be produced when RequestData() is called. See SetColumnStatus() for more information.
   */
  virtual int RequestSelectedColumns();

  /**
   * Empty the list of current requests.
   */
  virtual void ResetRequests();

  /**
   * Return the number of requests.
   * This does not include any request that is in the column-status buffer
   * but for which RequestSelectedColumns() has not yet been called (even though
   * it is possible this request will be honored when the filter is run -- see SetColumnStatus()
   * for more information).
   */
  virtual svtkIdType GetNumberOfRequests();

  /**
   * Return the number of columns for a given request.
   */
  virtual svtkIdType GetNumberOfColumnsForRequest(svtkIdType request);

  /**
   * Provide the name of the \a c-th column for the \a r-th request.

   * For the version of this routine that returns an integer,
   * if the request or column does not exist because \a r or \a c is out of bounds,
   * this routine returns 0 and the value of \a columnName is unspecified.
   * Otherwise, it returns 1 and the value of \a columnName is set.

   * For the version of this routine that returns const char*,
   * if the request or column does not exist because \a r or \a c is out of bounds,
   * the routine returns nullptr. Otherwise it returns the column name.
   * This version is not thread-safe.
   */
  virtual const char* GetColumnForRequest(svtkIdType r, svtkIdType c);

  virtual int GetColumnForRequest(svtkIdType r, svtkIdType c, svtkStdString& columnName);

  /**
   * Convenience method to create a request with a single column name \p namCol in a single
   * call; this is the preferred method to select columns, ensuring selection consistency
   * (a single column per request).
   * Warning: no name checking is performed on \p namCol; it is the user's
   * responsibility to use valid column names.
   */
  void AddColumn(const char* namCol);

  /**
   * Convenience method to create a request with a single column name pair
   * (\p namColX, \p namColY) in a single call; this is the preferred method to select
   * columns pairs, ensuring selection consistency (a pair of columns per request).

   * Unlike SetColumnStatus(), you need not call RequestSelectedColumns() after AddColumnPair().

   * Warning: \p namColX and \p namColY are only checked for their validity as strings;
   * no check is made that either are valid column names.
   */
  void AddColumnPair(const char* namColX, const char* namColY);

  /**
   * A convenience method (in particular for access from other applications) to
   * set parameter values of Learn mode.
   * Return true if setting of requested parameter name was executed, false otherwise.
   * NB: default method (which is sufficient for most statistics algorithms) does not
   * have any Learn parameters to set and always returns false.
   */
  virtual bool SetParameter(const char* parameter, int index, svtkVariant value);

  /**
   * Given a collection of models, calculate aggregate model
   */
  virtual void Aggregate(svtkDataObjectCollection*, svtkMultiBlockDataSet*) = 0;

protected:
  svtkStatisticsAlgorithm();
  ~svtkStatisticsAlgorithm() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Execute the calculations required by the Learn option, given some input Data
   */
  virtual void Learn(svtkTable*, svtkTable*, svtkMultiBlockDataSet*) = 0;

  /**
   * Execute the calculations required by the Derive option.
   */
  virtual void Derive(svtkMultiBlockDataSet*) = 0;

  /**
   * Execute the calculations required by the Assess option.
   */
  virtual void Assess(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) = 0;

  /**
   * A convenience implementation for generic assessment with variable number of variables.
   */
  void Assess(svtkTable*, svtkMultiBlockDataSet*, svtkTable*, int);

  /**
   * Execute the calculations required by the Test option.
   */
  virtual void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) = 0;

  /**
   * A pure virtual method to select the appropriate assessment functor.
   */
  virtual void SelectAssessFunctor(
    svtkTable* outData, svtkDataObject* inMeta, svtkStringArray* rowNames, AssessFunctor*& dfunc) = 0;

  svtkIdType NumberOfPrimaryTables;
  bool LearnOption;
  bool DeriveOption;
  bool AssessOption;
  bool TestOption;
  svtkStringArray* AssessNames;
  svtkStatisticsAlgorithmPrivate* Internals;

private:
  svtkStatisticsAlgorithm(const svtkStatisticsAlgorithm&) = delete;
  void operator=(const svtkStatisticsAlgorithm&) = delete;
};

#endif
