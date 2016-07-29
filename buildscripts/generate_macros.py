#!/usr/bin/python

# Copyright 2016 MongoDB Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Auto-generates the given number of levels of the MANGROVE_CHILD* macro.
# These are placed in a file with the proper mangrove boilerplate.
# This is used to to specify nested fields in the query builder.
# BSON supports up to 100 levels of nesting.

import StringIO
import sys

# variable for storing maximum BSON nesting depth, in case this changes.
MAX_BSON_DEPTH = 100

# A reference of what the generated macro should look like for n=3.
# This is for use in testing and documentation.
REFERENCE_MACRO_3 = (
    "#define MANGROVE_CHILD3(base, field1, field2, field3) "
    "make_nvp_with_parent(MANGROVE_KEY_BY_VALUE(&std::decay_t<typename decltype(MANGROVE_CHILD2("
    "base, field1, field2))::child_base_type>::field3), "
    "MANGROVE_CHILD2(base, field1, field2))")


def usage():
    print("Usage: python generate_macros.py <outfile> [max_depth]")
    print("Generates 'max_depth' levels of the MANGROVE_CHILD* macro, and write the output ")
    print("to 'outfile'. ")
    exit()


def make_mangrove_child(n):
    """
    Generates a cersion of the MANGROVE_CHILD* macro, with * = n.
    :param n: The nesting level of the MANGROVE_CHILD macro, i.e. how many 'fieldN'
              arguments it should accept.
    """

    if n < 2:
        raise ValueError("Argument n must be at least 2. Received n=%d." % n)

    prev_fields = ", ".join(["field" + str(i) for i in range(1, n)])
    all_fields = prev_fields + ", field" + str(n)
    s = ("#define MANGROVE_CHILD%d(base, %s) "
         "make_nvp_with_parent(MANGROVE_KEY_BY_VALUE(&std::decay_t<"
         "typename decltype(MANGROVE_CHILD%d(base, %s))::child_base_type>::field%d), "
         "MANGROVE_CHILD%d(base, %s))") % \
        (n, all_fields, n - 1, prev_fields, n, n - 1, prev_fields)
    return s


def test():
    assert(make_mangrove_child(3) == REFERENCE_MACRO_3)


def print_macros(outfile, max_n):
    """
    Prints the MANGROVE_CHILD* macros for *=2 to `max_n`.
    :param outfile: The file to write generated code to.
    :param max_n: The maximum level to which the macro is generated.
    """

    outfile.write(
        """// clang-format off
        // Copyright 2016 MongoDB Inc.
        //
        // Licensed under the Apache License, Version 2.0 (the "License");
        // you may not use this file except in compliance with the License.
        // You may obtain a copy of the License at
        //
        // http://www.apache.org/licenses/LICENSE-2.0
        //
        // Unless required by applicable law or agreed to in writing, software
        // distributed under the License is distributed on an "AS IS" BASIS,
        // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        // See the License for the specific language governing permissions and
        // limitations under the License.

        #pragma once

        #include <mangrove/config/prelude.hpp>

        // Preprocessor templates for manipulating multiple arguments.
        #define MANGROVE_PP_NARG(...) MANGROVE_PP_NARG_(__VA_ARGS__, MANGROVE_PP_RSEQ_N())
        #define MANGROVE_PP_NARG_(...) MANGROVE_PP_ARG_N(__VA_ARGS__)
        """)

    num_seq = "".join(["_%d, " % n for n in range(max_n + 2)])
    outfile.write("#define MANGROVE_PP_ARG_N(%s N, ...) N\n\n" % num_seq)

    num_seq = ", ".join([str(i) for i in range(max_n + 2, -1, -1)])
    outfile.write("#define MANGROVE_PP_RSEQ_N() %s\n\n" % num_seq)

    outfile.write("#define MANGROVE_CHILD1(base, field1) MANGROVE_KEY_BY_VALUE(&base::field1)\n\n")

    for i in range(2, max_n + 1):
        outfile.write(make_mangrove_child(i))
        outfile.write("\n\n")

    outfile.write(
        """

        #include <mangrove/config/postlude.hpp>
    // clang-format on"""
    )
    outfile.write("\n")


def main():
    # test generated macro against reference
    test()

    args = sys.argv[1:]
    if len(args) not in [1, 2]:
        usage()
    outfile = open(args[0], "w")
    if len(args) == 2:
        print_macros(outfile, int(args[1]))
    else:
        # default value corresponds to max nested document depth in BSON.
        print_macros(outfile, MAX_BSON_DEPTH)


if __name__ == "__main__":
    main()
