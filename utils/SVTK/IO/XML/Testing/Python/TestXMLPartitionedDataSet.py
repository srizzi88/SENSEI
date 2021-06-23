from svtkmodules import svtkCommonCore as cc
from svtkmodules import svtkCommonDataModel as dm
from svtkmodules import svtkCommonExecutionModel as em
from svtkmodules import svtkImagingCore as ic
from svtkmodules import svtkIOXML as ixml

from svtk.test import Testing
from svtk.util.misc import svtkGetTempDir

import os

class TestXMLPartitionedDataSet(Testing.svtkTest):

    def test(self):

        p = dm.svtkPartitionedDataSet()

        s = ic.svtkRTAnalyticSource()
        s.SetWholeExtent(0, 10, 0, 10, 0, 5)
        s.Update()

        p1 = dm.svtkImageData()
        p1.ShallowCopy(s.GetOutput())

        s.SetWholeExtent(0, 10, 0, 10, 5, 10)
        s.Update()

        p2 = dm.svtkImageData()
        p2.ShallowCopy(s.GetOutput())

        p.SetPartition(0, p1)
        p.SetPartition(1, p2)

        tmpdir = svtkGetTempDir()
        fname = tmpdir+"/testxmlpartds.vtpd"
        w = ixml.svtkXMLPartitionedDataSetWriter()
        w.SetInputData(p)
        w.SetFileName(fname)
        w.Write()

        r = ixml.svtkXMLPartitionedDataSetReader()
        r.SetFileName(fname)
        r.Update()
        o = r.GetOutputDataObject(0)

        print(o.IsA("svtkPartitionedDataSet"))
        np = o.GetNumberOfPartitions()
        self.assertEqual(np, 2)

        for i in range(np):
            d = o.GetPartition(i)
            d2 = p.GetPartition(i)
            self.assertTrue(d.IsA("svtkImageData"))
            self.assertEqual(d.GetNumberOfCells(), d2.GetNumberOfCells())
        os.remove(fname)

if __name__ == "__main__":
    Testing.main([(TestXMLPartitionedDataSet, 'test')])
