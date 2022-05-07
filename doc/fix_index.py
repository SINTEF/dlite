"""Simple script that updates a html file by adding anchors to headers.

Usage: python fix_index.py HTMLFILE

This script replaces headers like

    <h1>A history of everything </h1>

with

    <h1 id="a-history-of-everything">A history of everything</h1>
"""
import sys
import re


htmlfile = sys.argv[1]
with open(htmlfile, "rt") as f:
    buf = f.read()

lst = re.split(r"<h(.)>(.*)(</h.>)", buf)
s = []

for i in range(len(lst) // 4):
    s.append(lst[4*i])
    s.append(
        f'<h{lst[4*i+1]} id="{lst[4*i+2].strip().lower().replace(" ", "-")}">'
        f'{lst[4*i+2].strip()}{lst[4*i+3]}')
s.append(lst[-1])

with open(htmlfile, "wt") as f:
    f.write('\n'.join(s))
