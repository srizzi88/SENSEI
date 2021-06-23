from svtkmodules.svtkCommonCore import svtkInformation
from svtkmodules.svtkCommonDataModel import svtkDataObject
from svtkmodules.svtkCommonExecutionModel import svtkAlgorithm
from svtkmodules.svtkCommonExecutionModel import svtkDemandDrivenPipeline
from svtkmodules.svtkCommonExecutionModel import svtkStreamingDemandDrivenPipeline
from svtkmodules.svtkFiltersPython import svtkPythonAlgorithm

class SVTKAlgorithm(object):
    """This is a superclass which can be derived to implement
    Python classes that work with svtkPythonAlgorithm. It implements
    Initialize(), ProcessRequest(), FillInputPortInformation() and
    FillOutputPortInformation().

    Initialize() sets the input and output ports based on data
    members.

    ProcessRequest() calls RequestXXX() methods to implement
    various pipeline passes.

    FillInputPortInformation() and FillOutputPortInformation() set
    the input and output types based on data members.
    """

    def __init__(self, nInputPorts=1, inputType='svtkDataSet',
                       nOutputPorts=1, outputType='svtkPolyData'):
        """Sets up default NumberOfInputPorts, NumberOfOutputPorts,
        InputType and OutputType that are used by various initialization
        methods."""

        self.NumberOfInputPorts = nInputPorts
        self.NumberOfOutputPorts = nOutputPorts
        self.InputType = inputType
        self.OutputType = outputType

    def Initialize(self, svtkself):
        """Sets up number of input and output ports based on
        NumberOfInputPorts and NumberOfOutputPorts."""

        svtkself.SetNumberOfInputPorts(self.NumberOfInputPorts)
        svtkself.SetNumberOfOutputPorts(self.NumberOfOutputPorts)

    def GetInputData(self, inInfo, i, j):
        """Convenience method that returns an input data object
        given a vector of information objects and two indices."""

        return inInfo[i].GetInformationObject(j).Get(svtkDataObject.DATA_OBJECT())

    def GetOutputData(self, outInfo, i):
        """Convenience method that returns an output data object
        given an information object and an index."""
        return outInfo.GetInformationObject(i).Get(svtkDataObject.DATA_OBJECT())

    def RequestDataObject(self, svtkself, request, inInfo, outInfo):
        """Overwritten by subclass to manage data object creation.
        There is not need to overwrite this class if the output can
        be created based on the OutputType data member."""
        return 1

    def RequestInformation(self, svtkself, request, inInfo, outInfo):
        """Overwritten by subclass to provide meta-data to downstream
        pipeline."""
        return 1

    def RequestUpdateExtent(self, svtkself, request, inInfo, outInfo):
        """Overwritten by subclass to modify data request going
        to upstream pipeline."""
        return 1

    def RequestData(self, svtkself, request, inInfo, outInfo):
        """Overwritten by subclass to execute the algorithm."""
        raise NotImplementedError('RequestData must be implemented')

    def ProcessRequest(self, svtkself, request, inInfo, outInfo):
        """Splits a request to RequestXXX() methods."""
        if request.Has(svtkDemandDrivenPipeline.REQUEST_DATA_OBJECT()):
            return self.RequestDataObject(svtkself, request, inInfo, outInfo)
        elif request.Has(svtkDemandDrivenPipeline.REQUEST_INFORMATION()):
            return self.RequestInformation(svtkself, request, inInfo, outInfo)
        elif request.Has(svtkStreamingDemandDrivenPipeline.REQUEST_UPDATE_EXTENT()):
            return self.RequestUpdateExtent(svtkself, request, inInfo, outInfo)
        elif request.Has(svtkDemandDrivenPipeline.REQUEST_DATA()):
            return self.RequestData(svtkself, request, inInfo, outInfo)

        return 1

    def FillInputPortInformation(self, svtkself, port, info):
        """Sets the required input type to InputType."""
        info.Set(svtkAlgorithm.INPUT_REQUIRED_DATA_TYPE(), self.InputType)
        return 1

    def FillOutputPortInformation(self, svtkself, port, info):
        """Sets the default output type to OutputType."""
        info.Set(svtkDataObject.DATA_TYPE_NAME(), self.OutputType)
        return 1

class SVTKPythonAlgorithmBase(svtkPythonAlgorithm):
    """This is a superclass which can be derived to implement
    Python classes that act as SVTK algorithms in a SVTK pipeline.
    It implements ProcessRequest(), FillInputPortInformation() and
    FillOutputPortInformation().

    ProcessRequest() calls RequestXXX() methods to implement
    various pipeline passes.

    FillInputPortInformation() and FillOutputPortInformation() set
    the input and output types based on data members.

    Common use is something like this:

    class HDF5Source(SVTKPythonAlgorithmBase):
        def __init__(self):
            SVTKPythonAlgorithmBase.__init__(self,
                nInputPorts=0,
                nOutputPorts=1, outputType='svtkImageData')

        def RequestInformation(self, request, inInfo, outInfo):
            f = h5py.File("foo.h5", 'r')
            dims = f['RTData'].shape[::-1]
            info = outInfo.GetInformationObject(0)
            info.Set(svtkmodules.svtkCommonExecutionModel.svtkStreamingDemandDrivenPipeline.WHOLE_EXTENT(),
                (0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1), 6)
            return 1

        def RequestData(self, request, inInfo, outInfo):
            f = h5py.File("foo.h5", 'r')
            data = f['RTData'][:]
            output = dsa.WrapDataObject(svtkmodules.svtkCommonDataModel.svtkImageData.GetData(outInfo))
            output.SetDimensions(data.shape)
            output.PointData.append(data.flatten(), 'RTData')
            output.PointData.SetActiveScalars('RTData')
            return 1

    alg = HDF5Source()

    cf = svtkmodules.svtkFiltersCore.svtkContourFilter()
    cf.SetInputConnection(alg.GetOutputPort())
    cf.Update()
    """

    class InternalAlgorithm(object):
        "Internal class. Do not use."
        def Initialize(self, svtkself):
            pass

        def FillInputPortInformation(self, svtkself, port, info):
            return svtkself.FillInputPortInformation(port, info)

        def FillOutputPortInformation(self, svtkself, port, info):
            return svtkself.FillOutputPortInformation(port, info)

        def ProcessRequest(self, svtkself, request, inInfo, outInfo):
            return svtkself.ProcessRequest(request, inInfo, outInfo)

    def __init__(self, nInputPorts=1, inputType='svtkDataSet',
                       nOutputPorts=1, outputType='svtkPolyData'):
        """Sets up default NumberOfInputPorts, NumberOfOutputPorts,
        InputType and OutputType that are used by various methods.
        Make sure to call this method from any subclass' __init__"""

        self.SetPythonObject(SVTKPythonAlgorithmBase.InternalAlgorithm())

        self.SetNumberOfInputPorts(nInputPorts)
        self.SetNumberOfOutputPorts(nOutputPorts)

        self.InputType = inputType
        self.OutputType = outputType

    def GetInputData(self, inInfo, i, j):
        """Convenience method that returns an input data object
        given a vector of information objects and two indices."""

        return inInfo[i].GetInformationObject(j).Get(svtkDataObject.DATA_OBJECT())

    def GetOutputData(self, outInfo, i):
        """Convenience method that returns an output data object
        given an information object and an index."""
        return outInfo.GetInformationObject(i).Get(svtkDataObject.DATA_OBJECT())

    def FillInputPortInformation(self, port, info):
        """Sets the required input type to InputType."""
        info.Set(svtkAlgorithm.INPUT_REQUIRED_DATA_TYPE(), self.InputType)
        return 1

    def FillOutputPortInformation(self, port, info):
        """Sets the default output type to OutputType."""
        info.Set(svtkDataObject.DATA_TYPE_NAME(), self.OutputType)
        return 1

    def ProcessRequest(self, request, inInfo, outInfo):
        """Splits a request to RequestXXX() methods."""
        if request.Has(svtkDemandDrivenPipeline.REQUEST_DATA_OBJECT()):
            return self.RequestDataObject(request, inInfo, outInfo)
        elif request.Has(svtkDemandDrivenPipeline.REQUEST_INFORMATION()):
            return self.RequestInformation(request, inInfo, outInfo)
        elif request.Has(svtkStreamingDemandDrivenPipeline.REQUEST_UPDATE_EXTENT()):
            return self.RequestUpdateExtent(request, inInfo, outInfo)
        elif request.Has(svtkDemandDrivenPipeline.REQUEST_DATA()):
            return self.RequestData(request, inInfo, outInfo)

        return 1

    def RequestDataObject(self, request, inInfo, outInfo):
        """Overwritten by subclass to manage data object creation.
        There is not need to overwrite this class if the output can
        be created based on the OutputType data member."""
        return 1

    def RequestInformation(self, request, inInfo, outInfo):
        """Overwritten by subclass to provide meta-data to downstream
        pipeline."""
        return 1

    def RequestUpdateExtent(self, request, inInfo, outInfo):
        """Overwritten by subclass to modify data request going
        to upstream pipeline."""
        return 1

    def RequestData(self, request, inInfo, outInfo):
        """Overwritten by subclass to execute the algorithm."""
        raise NotImplementedError('RequestData must be implemented')
