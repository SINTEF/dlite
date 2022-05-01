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
