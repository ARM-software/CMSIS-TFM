#-------------------------------------------------------------------------------
# Copyright (c) 2018, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

import re
from keyword_substitution import keyword_substitute, log_print, Verbosity

tests = []
VERBOSITY = Verbosity.warning
log_print(Verbosity.debug, "Setting verbosity to", VERBOSITY, verbosity=VERBOSITY)
tests.append({
    "description": "Test no substitution",
    "template": "quickbrownfox",
    "db": {"a": "b", "c": "d"},
    "expected": "quickbrownfox\n"})
tests.append({
    "description": "Test simple substitution",
    "template": "@@a@@",
    "db": {"a": "b", "c": "d"},
    "expected": "b\n"})
tests.append({
    "template": "a123",
    "db": [{"c": "d", "a": {"a1": "lollipop", "a2": "PPAP"}},
           {"c": "x", "a": {"a1": "candyshop", "a2": "GitS: SAC"}}],
    "expected": "a123\na123\n"})
tests.append({
    "description": "Test two independent base symbols with two substitutions",
    "template": "@@a@@ @@c@@",
    "db": [{"a": "b", "c": "d"}, {"a": "x", "c": "x"}],
    "expected": "b d\n" +
                "x x\n"})
tests.append({
    "template": "@@a@@@ @b@@c@@",
    "db": [{"a": "b", "c": "d"}, {"a": "x", "c": "x"}],
    "expected": "b@ @bd\n" +
                "x@ @bx\n"})
tests.append({
    "template": "@@a.a1@@",
    "db": [{"a": {"a1": "lollipop", "a2": "PPAP"}, "c": "d"}],
    "expected": "lollipop\n"})
tests.append({
    "template": "@@a.a1@@ @@a.a2@@",
    "db": [{"a": {"a1": "lollipop", "a2": "PPAP"}, "c": "d"}, {"a": "x", "c": "x"}],
    "expected": "lollipop PPAP\n" +
                "x x\n"})
tests.append({
    "template": "@@a.a1@@ @@a.a2@@",
    "db": [{"a": {"a1": "lollipop", "a2": "PPAP"}, "c": "d"},
           {"a": {"a1": "candyshop", "a2": "GitS: SAC"}, "c": "x"}],
    "expected": "lollipop PPAP\n" +
                "candyshop GitS: SAC\n"})
tests.append({
    "template": "@@a.a1@@ @@a.a2@@ @@a@@",
    "db": [{"a": {"a1": "lollipop", "a2": "PPAP"}, "c": "d"}, {"a": {"a1": "candyshop", "a2": "GitS: SAC"}, "c": "x"}],
    "expected": "lollipop PPAP @@@@\n" +
                "candyshop GitS: SAC @@@@\n"})
tests.append({
    "description": "List in dict",
    "template": "@@a.name@@",
    "db": [{"a": [{"name": "inst1"}, {"name": "inst2"}]}],
    "expected": "inst1\n" +
                "inst2\n"})
tests.append({
    "description": "List in dict in list",
    "template": "@@name@@ subinstance: @@subinstlist.name@@",
    "db": [{"name": "inst1", "subinstlist": [{"name": "subinst1"}, {"name": "subinst2"}]},
           {"name": "inst2", "subinstlist": [{"name": "subinst3"}, {"name": "subinst4"}]}],
    "expected": "inst1 subinstance: subinst1\n" +
                "inst1 subinstance: subinst2\n" +
                "inst2 subinstance: subinst3\n" +
                "inst2 subinstance: subinst4\n"})
tests.append({
    "description": "Chain forks: multiple diverging chains",
    "template": "@@name@@ subinstance: @@subinstlist.name@@ @@subinst2.name@@",
    "db": {"name": "inst1", "subinstlist": [{"name": "subinst1"}, {"name": "subinst2"}],
                             "subinst2": [{"name": "xxx"}, {"name": "yyy"}]},
    "expected": "inst1 subinstance: subinst1 xxx\n" +
                "inst1 subinstance: subinst1 yyy\n" +
                "inst1 subinstance: subinst2 xxx\n" +
                "inst1 subinstance: subinst2 yyy\n"})
tests.append({
    "description": "Chain forks: multiple diverging chains",
    "template": "@@name@@: @@subinstlist.name@@ @@subinstlist2.name@@ @@subinstlist3.name@@",
    "db": {"name": "inst1", "subinstlist1": [{"name": "subinst1"}, {"name": "subinst2"}],
                            "subinstlist2": [{"name": "xxx"}, {"name": "yyy"}],
                            "subinstlist3": [{"name": "aaa"}, {"name": "bbb"}]},
    "expected": "inst1: @@subinstlist.name@@ xxx aaa\n" +
                "inst1: @@subinstlist.name@@ xxx bbb\n" +
                "inst1: @@subinstlist.name@@ yyy aaa\n" +
                "inst1: @@subinstlist.name@@ yyy bbb\n"})
tests.append({
    "description": "Chain forks: multiple diverging chains, array of root instances",
    "template": "@@name@@ subinstance: @@subinstlist.name@@ @@subinst2.name@@",
    "db": [{"name": "inst1", "subinstlist": [{"name": "subinst1"}, {"name": "subinst2"}],
                             "subinst2": [{"name": "xxx"}, {"name": "yyy"}]},
           {"name": "inst2", "subinstlist": [{"name": "subinst3"}, {"name": "subinst4"}],
                             "subinst2": [{"name": "inst2subinstlist2subinst1"}, {"name": "inst2subinstlist2subinst2"}]}],
    "expected": "inst1 subinstance: subinst1 xxx\n" +
                "inst1 subinstance: subinst1 yyy\n" +
                "inst1 subinstance: subinst2 xxx\n" +
                "inst1 subinstance: subinst2 yyy\n" +
                "inst2 subinstance: subinst3 inst2subinstlist2subinst1\n" +
                "inst2 subinstance: subinst3 inst2subinstlist2subinst2\n" +
                "inst2 subinstance: subinst4 inst2subinstlist2subinst1\n" +
                "inst2 subinstance: subinst4 inst2subinstlist2subinst2\n"})
tests.append({
    "description": "Chain forks: multiple diverging chains with missing subinstance",
    "template": "@@name@@ subinstance: @@subinstlist1.name@@ @@subinstlist2.name@@",
    "db": [{"name": "inst1", "subinstlist1": [{"name": "subinst1"}, {"name": "subinst2"}],
                             "subinstlist2": [{"name": "xxx"}, {"name": "yyy"}]},
           {"name": "inst2", "subinstlist1": [{"name": "subinst3"}, {"name": "subinst4"}]}],
    "expected": "inst1 subinstance: subinst1 xxx\n"
                "inst1 subinstance: subinst1 yyy\n"
                "inst1 subinstance: subinst2 xxx\n"
                "inst1 subinstance: subinst2 yyy\n"
                "inst2 subinstance: subinst3 @@subinstlist2.name@@\n"
                "inst2 subinstance: subinst4 @@subinstlist2.name@@\n"})

def testsuite():
    for tcidx, test in enumerate(tests):
        print
        log_print(Verbosity.debug, "template:", test["template"])
        log_print(Verbosity.debug, "db:", test["db"])
        outlist = keyword_substitute(test["db"], test["template"], "report")
        log_print(Verbosity.debug, outlist)
        outstring = ""
        for outline in outlist:
            outstring += ''.join(outline) + "\n"
        log_print(Verbosity.info, "Got:")
        log_print(Verbosity.info, outstring)
        if outstring == test["expected"]:
            print "Test", tcidx, "PASSED"
            test["result"] = "PASSED"
        else:
            print "Test", tcidx, "FAILED, expected:"
            print test["expected"]
            test["result"] = "FAILED"

    print
    print "Test summary:"
    for idx, test in enumerate(tests):
        print "Test", idx, test["result"]

def main():
    testsuite()

if __name__ == "__main__":
    main()
