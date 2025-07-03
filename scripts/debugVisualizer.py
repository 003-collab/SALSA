'''
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
'''
# To use this script with QtCreator:
# Tools->Options->Debugger->GDB->Extra Debugging Helpers: click Browse and select this file
# ie: /home/<username>/projects/lsa/scripts/debugVisualizer.py

# Helpful resources:
# The Dumper object used by QtCreator can be found at $HOME/qtcreator-4.2.0/share/qtcreator/debugger/dumper.py
# https://doc.qt.io/qtcreator/creator-debugging-helpers.html
# https://sourceware.org/gdb/onlinedocs/gdb/Python-API.html#Python-API
# http://stackoverflow.com/questions/31124259/making-a-gdb-debugging-helper-for-the-quuid-class
# http://plohrmann.blogspot.com/2013/10/writing-debug-visualizers-for-gdb.html

from dumper import *
import gdb

def qdump__gnsstk__Matrix(d, value):

    # Get the array size and pointer to first element
    n = int(value["s"])
    rows= int(value["r"])
    cols= int(value["c"])
    vector = value["v"]
    array = vector["v"]
    address = array.address()
    pointer = array.pointer()

    # Get the type of the array
    arrayType = array.type.unqualified()
    innerType = arrayType.ltarget
    if innerType is None:
        innerType = array.type.target().unqualified()

    # Add array header
    d.putValue("(%dx%d), ColumnMajor" %(rows,cols))

    # Add child nodes
    d.putNumChild(n)
    rowNum = 0
    colNum = 0
    if d.isExpanded():
        with Children(d):
            for i in range(0,n):
                d.putSubItem("[%d,%d]" %(rowNum,colNum), array[i])
                rowNum += 1
                if rowNum == rows:
                    rowNum = 0
                    colNum += 1

def qdump__gnsstk__Vector(d, value):

    # Get the array size and pointer to first element
    n = int(value["s"])
    value = value["v"]
    address = value.address()
    pointer = value.pointer()

    # Get the type of the array
    arrayType = value.type.unqualified()
    innerType = arrayType.ltarget
    if innerType is None:
        innerType = value.type.target().unqualified()

    # Add header
    d.putValue("GNSSTk::Vector of length %d" %(n))

    # Add child nodes
    d.putNumChild(n)
    if d.isExpanded():
       d.putArrayData(pointer, n, innerType)

