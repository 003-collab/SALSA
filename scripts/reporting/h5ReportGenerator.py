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
#!/usr/bin/env python3

###
# Combine the h5CSV, h5PTS, h5PCR scripts and eliminate overhead in opening h5 file
###

import argparse

import h5PTS
import h5PCR
import h5CSV


def generateReports(h5file, cfgfile, pcrfile, ptsfile, csvfile, cgeoid, geoidfile2, apriori):
    if None not in [h5file, cfgfile, csvfile]:
        h5CSV.generateCSV(h5file, cfgfile, csvfile, cgeoid, geoidfile2, apriori)

    if None not in [h5file, cfgfile, ptsfile]:
        h5PTS.generatePTS(h5file, cfgfile, ptsfile)

    if None not in [h5file, cfgfile, pcrfile]:
        h5PCR.generatePCR(h5file, cfgfile, pcrfile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('--h5', action='store', dest='h5file', required=True)
    parser.add_argument('--cfg', action='store', dest='cfgfile', required=True)
    parser.add_argument('--pcr', action='store', dest='pcrfile', required=False)
    parser.add_argument('--pts', action='store', dest='ptsfile', required=False)
    parser.add_argument('--csv', action='store', dest='csvfile', required=False)
    parser.add_argument('--cgeoid', action='store', dest='cgeoid', required=False)
    parser.add_argument('--geo', action='store', dest='geoidfile2', required=False)
    parser.add_argument('--apriori', dest='apriori', action='store_true', required=False)
    parser.add_argument('--args', dest='showargs', action='store_true', required=False)
    parser.set_defaults(apriori=False)
    parser.set_defaults(showargs=False)

    args = parser.parse_args()

    generateReports(args.h5file, args.cfgfile, args.pcrfile, args.ptsfile, args.csvfile, args.cgeoid, args.geoidfile2, args.apriori)

    if args.showargs:
        print(args)
