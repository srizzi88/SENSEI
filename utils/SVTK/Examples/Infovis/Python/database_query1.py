#!/usr/bin/env python
from __future__ import print_function
from svtk import *
import os.path
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = SVTK_DATA_ROOT + "/Data/Infovis/SQLite/"
sqlite_file = data_dir + "SmallEmailTest.db"

database = svtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

query = database.GetQueryInstance()
query.SetQuery("select Name, Job, Age from employee")

queryToTable = svtkRowQueryToTable()
queryToTable.SetQuery(query)
queryToTable.Update()

T = queryToTable.GetOutput()

print("Query Results:")
T.Dump(12)

database.FastDelete()

