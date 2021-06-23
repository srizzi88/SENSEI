#ifndef svtkKMeansDistanceFunctor_h
#define svtkKMeansDistanceFunctor_h

/**
 * @class   svtkKMeansDistanceFunctor
 * @brief   measure distance from k-means cluster centers
 *
 * This is an abstract class (with a default concrete subclass) that implements
 * algorithms used by the svtkKMeansStatistics filter that rely on a distance metric.
 * If you wish to use a non-Euclidean distance metric (this could include
 * working with strings that do not have a Euclidean distance metric, implementing
 * k-mediods, or trying distance metrics in norms other than L2), you
 * should subclass svtkKMeansDistanceFunctor.
 */

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkObject.h"

class svtkVariantArray;
class svtkAbstractArray;
class svtkTable;

class SVTKFILTERSSTATISTICS_EXPORT svtkKMeansDistanceFunctor : public svtkObject
{
public:
  static svtkKMeansDistanceFunctor* New();
  svtkTypeMacro(svtkKMeansDistanceFunctor, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return an empty tuple. These values are used as cluster center coordinates
   * when no initial cluster centers are specified.
   */
  virtual svtkVariantArray* GetEmptyTuple(svtkIdType dimension);

  /**
   * Compute the distance from one observation to another, returning the distance
   * in the first argument.
   */
  virtual void operator()(double&, svtkVariantArray*, svtkVariantArray*);

  /**
   * This is called once per observation per run per iteration in order to assign the
   * observation to its nearest cluster center after the distance functor has been
   * evaluated for all the cluster centers.

   * The distance functor is responsible for incrementally updating the cluster centers
   * to account for the assignment.
   */
  virtual void PairwiseUpdate(svtkTable* clusterCenters, svtkIdType row, svtkVariantArray* data,
    svtkIdType dataCardinality, svtkIdType totalCardinality);

  /**
   * When a cluster center (1) has no observations that are closer to it than other cluster centers
   * or (2) has exactly the same coordinates as another cluster center, its coordinates should be
   * perturbed. This function should perform that perturbation.

   * Since perturbation relies on a distance metric, this function is the responsibility of the
   * distance functor.
   */
  virtual void PerturbElement(svtkTable*, svtkTable*, svtkIdType, svtkIdType, svtkIdType, double);

  /**
   * Allocate an array large enough to hold \a size coordinates and return a void pointer to this
   * array. This is used by svtkPKMeansStatistics to send (receive) cluster center coordinates to
   * (from) other processes.
   */
  virtual void* AllocateElementArray(svtkIdType size);

  /**
   * Free an array allocated with AllocateElementArray.
   */
  virtual void DeallocateElementArray(void*);

  /**
   * Return a svtkAbstractArray capable of holding cluster center coordinates.
   * This is used by svtkPKMeansStatistics to hold cluster center coordinates sent to (received from)
   * other processes.
   */
  virtual svtkAbstractArray* CreateCoordinateArray();

  /**
   * Pack the cluster center coordinates in \a vElements into columns of \a curTable.
   * This code may assume that the columns in \a curTable are all of the type returned by \a
   * GetNewSVTKArray().
   */
  virtual void PackElements(svtkTable* curTable, void* vElements);

  //@{
  /**
   * Unpack the cluster center coordinates in \a vElements into columns of \a curTable.
   * This code may assume that the columns in \a curTable are all of the type returned by \a
   * GetNewSVTKArray().
   */
  virtual void UnPackElements(
    svtkTable* curTable, svtkTable* newTable, void* vLocalElements, void* vGlobalElements, int np);
  virtual void UnPackElements(
    svtkTable* curTable, void* vLocalElements, svtkIdType numRows, svtkIdType numCols);
  //@}

  /**
   * Return the data type used to store cluster center coordinates.
   */
  virtual int GetDataType();

protected:
  svtkKMeansDistanceFunctor();
  ~svtkKMeansDistanceFunctor() override;

  svtkVariantArray* EmptyTuple; // Used to quickly initialize Tuple for each datum
  svtkTable*
    CenterUpdates; // Used to hold online computation of next iteration's cluster center coords.

private:
  svtkKMeansDistanceFunctor(const svtkKMeansDistanceFunctor&) = delete;
  void operator=(const svtkKMeansDistanceFunctor&) = delete;
};

#endif // svtkKMeansDistanceFunctor_h
