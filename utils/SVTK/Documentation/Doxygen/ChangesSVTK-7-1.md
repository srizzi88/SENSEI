Changes in SVTK 7.1          {#SVTK-7-1-Changes}
==================

This page documents API and behavior changes between SVTK 7.0 and
SVTK 7.1

Pipeline Update Methods
-----------------------

The following methods were deprecated in SVTK 7.1:

### svtkAlgorithm:

    int SetUpdateExtentToWholeExtent(int port);
    int SetUpdateExtentToWholeExtent();
    void SetUpdateExtent(int port,
                         int piece,int numPieces, int ghostLevel);
    void SetUpdateExtent(
      int piece,int numPieces, int ghostLevel);
    void SetUpdateExtent(int port, int extent[6]);
    void SetUpdateExtent(int extent[6]);

### svtkStreamingDemandDrivenPipeline:

    int SetUpdateExtentToWholeExtent(int port);
    static int SetUpdateExtentToWholeExtent(svtkInformation *);
    int SetUpdateExtent(int port, int extent[6]);
    int SetUpdateExtent(int port, int x0, int x1, int y0, int y1, int z0, int z1);
    static int SetUpdateExtent(svtkInformation *, int extent[6]);
    int SetUpdateExtent(int port,
                        int piece, int numPieces, int ghostLevel);
    static int SetUpdateExtent(svtkInformation *,
                               int piece, int numPieces, int ghostLevel);
    static int SetUpdatePiece(svtkInformation *, int piece);
    static int SetUpdateNumberOfPieces(svtkInformation *, int n);
    static int SetUpdateGhostLevel(svtkInformation *, int n);
    int SetUpdateTimeStep(int port, double time);
    static int SetUpdateTimeStep(svtkInformation *, double time);

The following new methods were added:

### svtkAlgorithm:

    int Update(int port, svtkInformationVector* requests);
    int Update(svtkInformation* requests);
    int UpdatePiece(int piece, int numPieces, int ghostLevels, const int extents[6]=0);
    int UpdateExtent(const int extents[6]);
    int UpdateTimeStep(double time,
        int piece=-1, int numPieces=1, int ghostLevels=0, const int extents[6]=0);

### svtkStreamingDemandDrivenPipeline:

    int Update(int port, svtkInformationVector* requests);

The main reason behind these changes is to make requesting a particular time step or a particular spatial subset (extent or pieces) during an update easier and more predictable. Prior to these changes, the following is the best way to request a subset during update:

    svtkNew<svtkRTAnalyticSource> source;
    // Set some properties of source here
    source->UpdateInformation();
    source->SetUpdateExtent(0, 5, 0, 5, 2, 2);
    source->Update();

Note that the following will not work:

    svtkNew<svtkRTAnalyticSource> source;
    // Set some properties of source here
    // source->UpdateInformation(); <-- this was commented out
    source->SetUpdateExtent(0, 5, 0, 5, 2, 2);
    source->Update();

This is because when the output of an algorithm is initialized, all request meta-data stored in its OutputInformation is removed. The initialization of the output happens during the first *RequestInformation*, which is why `UpdateInformation()` needs to be called before setting any request values. To make things more complicated, the following will also not work:

    svtkNew<svtkRTAnalyticSource> source;
    // Set some properties of source here
    source->UpdateInformation();
    source->SetUpdateExtent(0, 5, 0, 5, 2, 2);
    source->Modified();
    source->Update();

This is because during *RequestInformation*, the extent and piece requests are initialized to default values (which is the whole dataset) and *RequestInformation* is called during update if the algorithm has been modified since the last information update.

This necessary sequence of calls has been mostly tribal knowledge and is very error prone. To simplify pipeline updates with requests, we introduced a new set of methods. With the new API, our example would be:

    svtkNew<svtkRTAnalyticSource> source;
    int updateExtent[6] = {0, 5, 0, 5, 2, 2};
    // Set some properties of source here
    source->UpdateExtent(updateExtent);

To ask for a particular time step from a time source, we would do something like this:

    svtkNew<svtkExodusIIReader> reader;
    // Set properties here
    reader->UpdateTimeStep(0.5);
    // or
    reader->UpdateTimeStep(0.5, 1, 2, 1);

The last call asks for time value 0.5 and the first of two pieces with one ghost level.

The new algorithm also supports directly passing a number of keys to make requests:

    typedef svtkStreamingDemandDrivenPipeline svtkSDDP;
    svtkNew<svtkRTAnalyticSource> source;
    int updateExtent[6] = {0, 5, 0, 5, 2, 2};
    svtkNew<svtkInformation> requests;
    requests->Set(svtkSDDP::UPDATE_EXTENT(), updateExtent, 6);
    reader->Update(requests.GetPointer());

This is equivalent to:

    typedef svtkStreamingDemandDrivenPipeline svtkSDDP;
    svtkNew<svtkRTAnalyticSource> source;
    int updateExtent[6] = {0, 5, 0, 5, 2, 2};
    source->UpdateInformation();
    source->GetOutputInformation(0)->Set(svtkSDDP::UPDATE_EXTENT(), updateExtent, 6);
    reader->Update();

We expect to remove the deprecated methods in SVTK 8.0.

Derivatives
-----------

SVTK has a C/row-major ordering of arrays. The svtkCellDerivatives
filter was erroneously outputting second order tensors
(i.e. 9 component tuples) in Fortran/column-major ordering. This has been
fixed along with the numpy vector_gradient and strain functions.
Additionally, svtkTensors was removed as this class was only
used by svtkCellDerivatives and was contributing to the confusion.

svtkSMPTools
-----------

The following back-ends have been removed:
+ Simple: This is not a production level backend and was only used for debugging purposes.
+ Kaapi: This backend is no longer maintained.

svtkDataArray Refactor, svtkArrayDispatch and Related Tools
---------------------------------------------------------

The `svtkDataArrayTemplate` template class has been replaced by
`svtkAOSDataArrayTemplate` to distinguish it from the new
`svtkSOADataArrayTemplate`. The former uses Array-Of-Structs component ordering
while the latter uses Struct-Of-Arrays component ordering. These both derive
from the new `svtkGenericDataArray` template class and are an initial
implementation of native support for alternate memory layouts in SVTK.

To facilitate working with these arrays efficiently, several new tools have
been added in this release. They are detailed \ref SVTK-7-1-ArrayDispatch "here".

As part of the refactoring effort, several `svtkDataArrayTemplate` methods were
deprecated and replaced with new, const-correct methods with more meaningful
names.

The old and new method names are listed below:

+ `GetTupleValue` is now `GetTypedTuple`
+ `SetTupleValue` is now `SetTypedTuple`
+ `InsertTupleValue` is now `InsertTypedTuple`
+ `InsertNextTupleValue` is now `InsertNextTypedTuple`
