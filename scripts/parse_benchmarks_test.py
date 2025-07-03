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
# -*- coding: utf-8 -*-
"""
Created on Wed Oct 26 15:02:21 2016

@author: jsong
"""
import matplotlib
matplotlib.use('Agg')

import os
import re
import subprocess
import time

import socket
import sqlite3


import matplotlib.pyplot

SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))

LSA_ROOT = os.path.join(SCRIPT_PATH, '../')
BENCHMARKS_EXECUTABLE = os.path.join(LSA_ROOT, 'build/benchmarks')

os.chdir(LSA_ROOT)


def run_benchmark_executable():
    # TODO: what's the appropriate style?
    with subprocess.Popen([BENCHMARKS_EXECUTABLE],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE) as proc:
    
        stderr = proc.stderr.read().decode()
        stdout = proc.stdout.read().decode()
        
    return stdout, stderr


def get_git_revision():
    git_args = ['git', 'rev-parse', 'HEAD']

    with subprocess.Popen(git_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        stderr = proc.stderr.read().decode()
        stdout = proc.stdout.read().decode().replace('\n', '')

    return stdout


def get_git_branch():
    git_args = ['git', 'symbolic-ref', 'HEAD']

    with subprocess.Popen(git_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        stderr = proc.stderr.read().decode()
        stdout = proc.stdout.read().decode().replace('\n', '')

    branch_name_split = stdout.split('/')
    branch_name = branch_name_split[len(branch_name_split) - 1]

    return branch_name


def process_benchmark_stdout(stdout):
    matches = re.findall(r'[^ ]+(?= msecs per iteration)', stdout)
    keys = re.findall(r'(?<=RESULT : ).+', stdout)
    zipped = list(zip(keys, matches))

    output = {element[0][:-1]: float(element[1].replace(',', '')) for element in zipped}

    return output


def generate_data_dict():
    benchmark_stdout, benchmark_stderr = run_benchmark_executable()
    results = process_benchmark_stdout(benchmark_stdout)
    git_revision = get_git_revision()
    git_branch = get_git_branch()

    output = {'timestamp': int(time.time()),
              'branch': git_branch,
              'revision': git_revision,
              'results': results}
    
    return output


def initialize_sqlite(dbname):
    conn = sqlite3.connect(dbname)
    cursor = conn.cursor()

    # TODO: relational table for the branch, revision, timestamp columns
    create_query = '''CREATE TABLE results
                          (id INTEGER PRIMARY KEY AUTOINCREMENT,
                           hostname TEXT,
                           branch TEXT,
                           revision TEXT,
                           timestamp INTEGER,
                           testname TEXT,
                           value REAL)'''
    
    
    cursor.execute(create_query)
    
    conn.commit()
    conn.close()


def insert_row(dbname, data):
    conn = sqlite3.connect(dbname)
    cursor = conn.cursor()

    for testname in data['results']:    
    
        insert_query = '''INSERT INTO results
                              (hostname, branch, revision, timestamp, testname, value)
                          VALUES
                              (?, ?, ?, ?, ?, ?)'''
    
        values = (socket.gethostname(),
                  data['branch'],
                  data['revision'],
                  data['timestamp'],
                  testname,
                  data['results'][testname])                          

        cursor.execute(insert_query, values)

    conn.commit()
    conn.close()


def get_all_rows(dbname):

    def dict_factory(cursor, row):
        d = {}
        for idx, col in enumerate(cursor.description):
            d[col[0]] = row[idx]
        return d

    conn = sqlite3.connect(dbname)
    conn.row_factory = dict_factory

    cursor = conn.cursor()
    
    select_query = '''SELECT * FROM results'''
    
    cursor.execute(select_query)
    results = cursor.fetchall()
    
    return results


dbname = os.path.join(os.environ.get('LSA_OUT'), 'performance_metrics', 'lsa.sqlite')

def run_and_insert(dbname):
    data = generate_data_dict()
    insert_row(dbname, data)

def plot_results(dbname, outfile=None):
    species = {}
    results = get_all_rows(dbname)
    
    for result in results:
        if result['testname'] not in species:
            species[result['testname']] = [[], [], []]

        species[result['testname']][0].append(result['timestamp'])
        species[result['testname']][1].append(result['value'])
        species[result['testname']][2].append(result['revision'][:7])

    fig = matplotlib.pyplot.figure()
    ax = fig.add_subplot(111)
    for testname in species:
        indices = range(len(species[testname][1]))
        ax.plot(indices, species[testname][1], label=testname, linestyle='-', marker='.')
        ax.set_xticks(indices)
        ax.set_xticklabels(species[testname][2], rotation='vertical')

    ax.grid(True)
    ax.legend(loc=1)

    fig.tight_layout()
    if outfile is not None:
        fig.savefig(os.path.join('build', outfile), format='png', dpi=300)
    
    return species

if __name__ == '__main__':

    import argparse
    parser = argparse.ArgumentParser(description="parse benchmarks")
    parser.add_argument('--out', action='store', dest='outfile', required=False)
    args = parser.parse_args()

    if args.outfile is not None:
        # print(str(get_all_rows(dbname)))
        plot_results(dbname, args.outfile)
    else:
        if not os.path.exists(dbname):
            initialize_sqlite(dbname)
          
        run_and_insert(dbname)

