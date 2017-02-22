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

# ensure we run from the root dir of the repo
ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))
os.chdir(ROOT)
GENERATED_DIR = os.path.join(ROOT, "generated_probers")

class CXXLanguage:
  def name(self):
    return "cpp"
  def create_makefile(self, uniquename):
    makefile = open("BUILD", "w")
    template = open("../../template/BUILD.cpp.template", "r").read()
    makefile.write(template.format(uniquename=uniquename))
    makefile.close()
  def do_prework(self, uniquename):
    print("c++ pre work")
    self.create_makefile(uniquename)
    cmd = ["protoc", "-I", ".", "--cpp_out=.", uniquename + ".proto"]
    proc = subprocess.Popen(args=cmd)
    proc.wait()
    cmd = ["protoc", "-I", ".", "--grpc_out=.", "--plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin", uniquename + ".proto"]
    proc = subprocess.Popen(args=cmd)
    proc.wait()
  def generate_client(self, uniquename):
    print("c++ main work")
    cmd = ["protoc", "-I", ".", "--grpc_out=.", "--plugin=protoc-gen-grpc=../../bazel-bin/cpp_generator", uniquename + ".proto"]
    proc = subprocess.Popen(args=cmd)
    proc.wait()

class GoLanguage:
  def name(self):
    return "go"
  def ensure_gogendir_exists(self):
    dirpath = os.path.expandvars("$GOPATH/src/generated_pb_files")
    if not os.path.exists(dirpath):
      os.makedirs(dirpath)
  def generate_pb_files(self, uniquename):
    # TODO(ncteisen) check for GOPATH bin stuff
    cmd = ["protoc", "-I", ".", "--go_out=plugins=grpc:.", uniquename + ".proto"]
    proc = subprocess.Popen(args=cmd)
    proc.wait()
    dirpath = os.path.expandvars("$GOPATH/src/generated_pb_files/" + uniquename)
    if os.path.exists(dirpath):
      shutil.rmtree(dirpath)
    os.mkdir(dirpath)
    shutil.move(uniquename + ".pb.go", dirpath + "/" + uniquename + ".pb.go")
  def do_prework(self, uniquename):
    print("go pre work")
    self.ensure_gogendir_exists()
    self.generate_pb_files(uniquename)
  def generate_client(self, uniquename):
    print("go main work")
    cmd = ["protoc", "-I", ".", "--grpc_out=.", 
        "--plugin=protoc-gen-grpc=../../bazel-bin/go_generator", 
        uniquename + ".proto"]
    proc = subprocess.Popen(args=cmd)
    proc.wait()

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

argp.add_argument('-p', '--proto',
                  required=True,
                  help='proto file from which to generate')

args = argp.parse_args()

# validate the proto arg
if not args.proto.endswith(".proto"):
  print("proto file needs to be .proto")
  raise SystemExit(1)

#TODO(ncteisen): check for protoc

languages = set(_LANGUAGES[l]
                for l in itertools.chain.from_iterable(
                      _LANGUAGES.iterkeys() if x == 'all' else [x]
                      for x in args.language))


# make the generators
proc = subprocess.Popen(args=["bazel", "build", ":all"])
proc.wait()

# generate the directories for each language
for lang in languages:
  os.chdir(ROOT) # back to root
  abspath = os.path.realpath(args.proto)
  os.chdir(GENERATED_DIR)
  uniquename = abspath.split("/")[-1][:-6]
  dirname = uniquename + "_" + lang.name()
  if os.path.exists(dirname):
    shutil.rmtree(dirname)
  os.mkdir(dirname)
  os.chdir(dirname)
  shutil.copyfile(abspath, os.getcwd() + "/" + uniquename + ".proto")
  lang.do_prework(uniquename)
  lang.generate_client(uniquename)

