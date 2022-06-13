# Quick and dirty pretty printer for federation types
# Use these while debugging a project that uses UDBM
# To use, add the following to your .gdbinit:
# python
# import sys
# sys.path.insert(0, 'UDBM_PATH')
# from udbm_pretty_printer import build_pretty_printer
# import gdb.printing
# gdb.printing.register_pretty_printer(
#     gdb.current_objfile(),
#     build_pretty_printer())
# end

import gdb
import gdb.printing
import re

dbm_INFINITY = 2**30 -1

def print_table(table):
    dim = len(table)
    column_lens = [0 for _ in range(dim)]
    for i in range(dim):
        for j in range(dim):
            l = len(table[j][i])
            if l > column_lens[i]:
                column_lens[i] = l
    out = ""
    for row in table:
        for i, el in enumerate(row):
            l = column_lens[i]
            out += el + " " + (l-len(el))*" "
        out += "\n"
    return out

class FederationPrinter:
    def __init__(self, val):
        self.val = val

    def size(self):
        x = self.val['ifedPtr']['fhead']
        s = 0
        while str(x) != "0x0":
            s += 1
            x = x['next']
        return s

    def to_string(self):
        fedSize = self.size()
        if fedSize == 0:
            return "Empty Federation"
        dim = int(self.val['ifedPtr']['dim'])
        out = "Federation of size " + str(fedSize) + ", dim " + str(dim) + ":"
        head = self.val['ifedPtr']['fhead']
        fed = []
        for _ in range(fedSize):
            dbm = str(head['idbm']['idbmPtr']['matrix'])
            arr = []
            for i in range(dim):
                row = []
                for j in range(dim):
                    raw = int(gdb.parse_and_eval(f"((int32_t*) {dbm})[{i}*{dim}+{j}]"))
                    bound = raw >> 1
                    if bound == dbm_INFINITY:
                        bound = "INF"
                    elif bound == -dbm_INFINITY:
                        bound = "-INF"
                    strict = raw & 1
                    op = "< "
                    if strict:
                        op = "<="
                    row.append(op + " " + str(bound))
                arr.append(row)
            head = head['next']
            fed.append(arr)
        return "and: \n".join([print_table(x) for x in fed])

    def display_hint(self):
        print("Federation")

def str_lookup_function(val):
    lookup_tag = val.type.tag
    if lookup_tag is None:
        return None
    regex = re.compile("dbm::fed_t")
    if regex.match(lookup_tag):
        return FederationPrinter(val)
    return lookup_tag

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("UDBM")
    pp.add_printer('fed_t', '^dbm::fed_t$', FederationPrinter)
    return pp
