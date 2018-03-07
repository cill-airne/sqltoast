#! /usr/bin/python

import argparse
import difflib
import os
import subprocess
import sys

TEST_DIR = os.path.dirname(os.path.realpath(__file__))
RESULT_OK = 0
RESULT_TEST_ERROR = 1
RESULT_TEST_FAILURE = 1
SQLTOASTER_BINARY = os.path.join(TEST_DIR, '..', '..', '_build', 'sqltoaster')


def get_dialects():
    """Returns a set of string dialect names for the supported grammar
    tests."""
    dialects = set()
    for fname in os.listdir(TEST_DIR):
        if os.path.isdir(os.path.join(TEST_DIR, fname)):
            dialects.add(fname)
    return dialects


def get_test_names(args):
    """Returns the list of filtered test names"""
    if args.dialect is not None:
        dialects = set(args.dialect)
    else:
        dialects = get_dialects()
    test_names = []
    for dialect in dialects:
        dpath = os.path.join(TEST_DIR, dialect)
        for fname in os.listdir(dpath):
            if fname.endswith(".test"):
                test_names.append("%s/%s" % (dialect, fname[:-5]))
    return sorted(test_names)


def run_test(test_name):
    test_path = os.path.join(TEST_DIR, test_name + ".test")
    input_blocks = []
    output_blocks = []
    input_block = []
    output_block = []
    with open(test_path, 'rb') as tfile:
        line = tfile.readline().rstrip("\n")
        while line:
            if not line:
                break;
            if line.startswith("#"):
                continue
            if line.startswith('>'):
                # Clear out previous output block...
                if output_block:
                    output_blocks.append("\n".join(output_block))
                    output_block = []
                input_block.append(line[1:])
            else:
                # Clear out previous input block...
                if input_block:
                    input_blocks.append("\n".join(input_block))
                    input_block = []
                output_block.append(line)
            line = tfile.readline().rstrip("\n")
    if output_block:
        output_blocks.append("\n".join(output_block))

    if len(input_blocks) != len(output_blocks):
        msg = ("Error in test file %s: expected same amount of input to "
               "output blocks but got %d input blocks and %d output blocks.")
        msg = msg % (test_name, len(input_blocks), len(output_blocks))
        return RESULT_TEST_ERROR, msg
    for testno, iblock in enumerate(input_blocks):
        expected = output_blocks[testno]
        cmd_args = [SQLTOASTER_BINARY, '--disable-timer', iblock]
        try:
            actual = subprocess.check_output(cmd_args)
        except subprocess.CalledProcessError as err:
            msg = ("Failed to execute test number %d inside %s. Got: %s")
            msg = msg % (testno, test_name, err)
            return RESULT_TEST_ERROR, msg

        actual = actual.rstrip("\n")

        if actual != expected:
            msg = "expected != actual\n"
            diff = difflib.unified_diff(expected, actual,
                                        fromfile="expected",
                                        tofile="actual")
            msg += "".join(list(diff))
            return RESULT_TEST_FAILURE, msg

    return RESULT_OK, None


def parse_options():
    """
    Parse any command line options and environs defaults and return an options
    object.
    """
    p = argparse.ArgumentParser(description="Run SQL grammar tests.")
    p.add_argument("command", default="run", choices=['run', 'list'])
    p.add_argument("--test-regex", "-r", default=None,
                   help="(optional) regex pattern to filter tests to run")
    p.add_argument("--dialect", choices=get_dialects(), default=None,
                   help="(optional) Only run SQL grammar tests for the "
                        "dialect specified. By default, all dialect tests "
                        "are run")

    return p.parse_args()


def command_list(args):
    """Lists tests that would meet any (optional) regex or dialect filters."""
    test_names = get_test_names(args)
    for tname in test_names:
        sys.stdout.write("%s\n" % tname)


def command_run(args):
    """Runs tests that meet any (optional) regex or dialect filters."""
    test_names = get_test_names(args)
    for tname in test_names:
        sys.stdout.write("Running %s ... " % tname)
        res, err = run_test(tname)
        if res == RESULT_OK:
            sys.stdout.write("OK\n")
        elif res == RESULT_TEST_ERROR:
            sys.stdout.write("Error: %s\n" % err)
        else:
            sys.stdout.write("FAIL\n")
            sys.stdout.write(err)


COMMAND_CALLBACKS = {
    'run': command_run,
    'list': command_list,
}


if __name__ == "__main__":
    args = parse_options()
    COMMAND_CALLBACKS[args.command](args)