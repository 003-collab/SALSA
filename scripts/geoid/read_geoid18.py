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
# This is intended as a script for testing Geoid18 read/write functionality
# and understanding the structure of the Geoid18 ascii file format.
# This functionality has been reimplemented in the Geoid() class found in geoid.cpp

# Geoid18 NGS web source: https://geodesy.noaa.gov/GEOID/GEOID18/downloads.shtml

# The Geoid18 heights grid is contained in /data/geoid/g2018u0.asc.  The grid
# ranges from 24N-58N latitude and 130W-60W longitude and has a spacing of
# 1 arcmin.  There are 2041=(34 deg)*60 deg/min + 1 rows, and 
# 4201=(70 deg)*60 deg/min + 1 columns in the grid.  The .asc file containing
# the heights values only has 8 values per line, so each row requires
# 526=4201 / 8 (int) + 4201 % 8 (mod) lines of text.

#! python3

import numpy as np
import linecache

np.set_printoptions(linewidth=200)

GEOIDFILE = '../../data/geoid/g2018u0.asc'
OUTFILE = 'geoid18.txt'
GEOID18_BOUNDS = (24., 58., 360.-130., 360.-60.)

def get_file_idx(row, col):
    '''
    526 lines per row
    8 columns per line
    one header line at beginning of file
    '''
    baserow = 526 * row
    row_offset = col // 8 # integer division
    file_col = col % 8 # modulo
    file_row = baserow + row_offset + 2 # don't forget header row
    return (file_row, file_col)

def get_geoid(minLt, maxLt, minLn, maxLn):
    gridrowmin = int(np.floor((minLt - GEOID18_BOUNDS[0]) * 60 + 0.5)) # padding
    gridrowmax = int(np.floor((maxLt - GEOID18_BOUNDS[0]) * 60 + 0.5)) # padding

    gridcolmin = int(np.floor((minLn - GEOID18_BOUNDS[2]) * 60 + 0.5)) # padding
    gridcolmax = int(np.floor((maxLn - GEOID18_BOUNDS[2]) * 60 + 0.5)) # padding

    # Dumb non-pythonic way
    rowscols = [] # list of (file_row, file_col) tuples
    for row in range(gridrowmax, gridrowmin-1, -1):
        for col in range(gridcolmin, gridcolmax+1):
            rowscols.append(get_file_idx(row, col))

    # just read heights straight in same order as rowscols
    heights = [0.0]*len(rowscols)
    for idx in range(len(rowscols)):
        filerow, filecol = rowscols[idx]
        line = linecache.getline(GEOIDFILE, filerow).split()
        heights[idx] = line[filecol]


    return heights

def grid_boundaries(minLt, maxLt, minLn, maxLn, numrows, numcols):
    rowstride = (maxLt - minLt) / numrows
    colstride = (maxLn - minLn) / numcols

    grid_list = [] # list of grid boundaries

    for row in range(numrows):
        for col in range(numcols):
            grid_list.append((
                                minLt + row*rowstride, 
                                minLt + (row+1)*rowstride - 1/60,
                                minLn + col*colstride,
                                minLn + (col+1)*colstride - 1/60
                            ))
    
    return grid_list


if __name__ == '__main__':
    heights = np.asarray(get_geoid(50, 51, 250, 251)).astype(float)
    print(len(heights))
    print(heights)
