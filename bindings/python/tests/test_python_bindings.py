#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import sys
from glob import glob
import unittest

thisdir = os.path.dirname(__file__)


# Wrap tests into a unittest TestCase
# This way, we can run the individual tests interactively
class ScriptTestCase(unittest.TestCase):
    def __init__(self, methodname='testfile', filename=None):
        unittest.TestCase.__init__(self, methodname)
        self.filename = filename

    def testfile(self):
        env = globals().copy()
        env.update(__file__=self.filename)
        with open(self.filename) as fd:
            try:
                exec(compile(fd.read(), self.filename, 'exec'), env)
            except SystemExit as exc:
                if exc.code == 44:
                    self.skipTest('exit code 44')
                else:
                    raise exc

    def id(self):
        return self.filename

    def __str__(self):
        return self.filename.split('tests/')[-1]

    def __repr__(self):
        return "ScriptTestCase(filename='%s')" % self.filename


def test(verbosity=1, stream=sys.stdout):
    tests = [test for test in sorted(glob(os.path.join(thisdir, 'test_*.py')))
             if not test.endswith('__.py')
             and not test.endswith('test_python_bindings.py')
             # Exclude test_global_dlite_state.py since the global state
             # that it is testing depends on the other tests.
             and not test.endswith('test_global_dlite_state.py')]
    ts = unittest.TestSuite()
    for test in tests:
        ts.addTest(ScriptTestCase(filename=os.path.abspath(test)))
    with open(os.devnull, 'w') as devnull:
        if not verbosity:
            stream = devnull
        ttr = unittest.TextTestRunner(verbosity=verbosity, stream=stream)

        # Redirect stdout and stderr to devnull
        # stderr is redicred at file-descriptor level to get rid of
        # C-level output
        dest_fd = devnull.fileno()
        stderr_fd = sys.stderr.fileno()
        # copy stderr_fd before it is overwritten
        # NOTE: `copied` is inheritable on Windows when duplicating a
        # standard stream
        with os.fdopen(os.dup(stderr_fd), 'wb') as copied:
            sys.stdout.flush()
            sys.stderr.flush()
            try:
                sys.stdout = devnull
                os.dup2(dest_fd, stderr_fd)  # $ exec >&dest
                results = ttr.run(ts)
            finally:
                sys.stdout.flush()
                sys.stderr.flush()
                sys.stdout = sys.__stdout__
                os.dup2(copied.fileno(), stderr_fd)  # $ exec >&copied
    return results


if __name__ == "__main__":
    for k in sorted(os.environ.keys()):
        for s in 'dlite', 'path', 'python':
            if s in k.lower():
                print("%35s : %-s" % (k, os.environ[k]))
    results = test(verbosity=2)
    if results.errors or results.failures:
        sys.exit(1)
