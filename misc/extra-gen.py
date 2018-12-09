# python level-gen.py <main layer> <extra layer> <description file>

import sys

f = open(sys.argv[1], 'r')
g = f.read().split()
l = []

for i in range(len(g)):
    r = g[i].split(',')
    for j in range(len(r)):
        if r[j] != '-1':
            l.append('%d,%d,%s,0,0,0,0,0' % (i, j, r[j]))

print('\n'.join(l))
