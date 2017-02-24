#!/usr/bin/env python2.7
# Copyright 2015, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
Helper script that handles all of the meta work that must be done in order
for client generation to occur for a particular language.
"""

from __future__ import print_function

import argparse
import os
import sys
import itertools
import subprocess
import shutil

def run_and_wait(cmd):
  proc = subprocess.Popen(args=cmd)
  proc.wait()

def run_and_check(cmd):
  proc = subprocess.Popen(args=cmd)
  ret = proc.wait()
  if ret:
    raise SystemExit(1)

class CXXLanguage:
  def name(self):
    return "cpp"
  def try_build(self, uniquename):
    run_and_check(["bazel", "build", ":generated_" + uniquename + "_prober"])

class GoLanguage:
  def name(self):
    return "go"
  def try_build(self, uniquename):
    run_and_check(["go", "build", uniquename + ".grpc.client.pb.go"])


_LANGUAGES = {
    'c++' : CXXLanguage(),
    'go' : GoLanguage(),
}


argp = argparse.ArgumentParser(description='Generate a probing client')
argp.add_argument('-l', '--language',
                  choices=['all'] + sorted(_LANGUAGES),
                  nargs='+',
                  default=['all'],
                  help='Clients languages to generate.')

args = argp.parse_args()

languages = set(_LANGUAGES[l]
                for l in itertools.chain.from_iterable(
                      _LANGUAGES.iterkeys() if x == 'all' else [x]
                      for x in args.language))

# ensure we run from the root dir of the repo
ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))
os.chdir(ROOT)

if 'all' in args.language:
  run_and_wait(["bazel", "build", ":all"])
else:
  for lang in languages:
    run_and_wait(["bazel", "build", ":" + lang.name() + "_generator"])

for filename in os.listdir('protos'):
  os.chdir(ROOT)
  uniquename = filename[:-6]
  run_and_wait(["python", "generate.py", "-p",
      "protos/" + filename, "-d", "tmp", 
      "--no-rebuild", "-l"] + args.language)

  for lang in languages:
    os.chdir(ROOT)
    os.chdir("tmp/" + uniquename + "_" + lang.name())
    lang.try_build(uniquename)


# shutil.rmtree("tmp")
