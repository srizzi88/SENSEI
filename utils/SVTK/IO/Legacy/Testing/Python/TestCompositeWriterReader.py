from svtkmodules import svtkCommonCore as cc
from svtkmodules import svtkCommonDataModel as dm
from svtkmodules import svtkCommonExecutionModel as em
from svtkmodules import svtkImagingCore as ic
from svtkmodules import svtkIOLegacy as il

from svtk.test import Testing
from svtk.util.misc import svtkGetTempDir

import os

class TestCompositeWriterReader(Testing.svtkTest):

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

        p2 = dm.svtkPartitionedDataSet()
        p2.ShallowCopy(p)

        c = dm.svtkPartitionedDataSetCollection()
        c.SetPartitionedDataSet(0, p)
        c.SetPartitionedDataSet(1, p2)

        tmpdir = svtkGetTempDir()
        fname = tmpdir+"/testcompowriread.svtk"
        w = il.svtkCompositeDataWriter()
        w.SetInputData(c)
        w.SetFileName(fname)
        w.Write()

        r = il.svtkCompositeDataReader()
        r.SetFileName(fname)
        r.Update()
        o = r.GetOutputDataObject(0)

        self.assertTrue(o.IsA("svtkPartitionedDataSetCollection"))
        nd = o.GetNumberOfPartitionedDataSets()
        self.assertEqual(nd, 2)

        for i in range(nd):
            p = o.GetPartitionedDataSet(i)
            p2 = c.GetPartitionedDataSet(i)
            self.assertTrue(p.IsA("svtkPartitionedDataSet"))
            self.assertEqual(p.GetNumberOfPartitions(), 2)
            self.assertEqual(p.GetPartition(0).GetNumberOfCells(), p.GetPartition(0).GetNumberOfCells())
        del(r)
        import gc
        gc.collect()
        os.remove(fname)


if __name__ == "__main__":
    Testing.main([(TestCompositeWriterReader, 'test')])
