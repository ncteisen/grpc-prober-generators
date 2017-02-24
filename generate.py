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

def check_path(binary):
  with open(os.devnull, "w") as devnull:
    proc = subprocess.Popen(args=["which", binary], stdout=devnull)
    ret = proc.wait()
    if ret:
      print("ensure that " + binary + " is in your $PATH")
      raise SystemExit(1)

class CXXLanguage:
  def name(self):
    return "cpp"
  def create_makefile(self, uniquename):
    makefile = open("BUILD", "w")
    template = open(ROOT + "/template/BUILD.cpp.template", "r").read()
    makefile.write(template.format(uniquename=uniquename))
    makefile.close()
  def do_prework(self, uniquename):
    print("c++ pre work")
    self.create_makefile(uniquename)
    run_and_wait(["protoc", "-I", ".", "--cpp_out=.", uniquename + ".proto"])
    run_and_wait(["protoc", "-I", ".", "--grpc_out=.", 
        "--plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin", 
        uniquename + ".proto"])
  def generate_client(self, uniquename):
    print("c++ main work")
    run_and_wait(["protoc", "-I", ".", "--grpc_out=.", 
        "--plugin=protoc-gen-grpc=../../bazel-bin/cpp_generator", 
        uniquename + ".proto"])

class GoLanguage:
  def name(self):
    return "go"
  def check_path(self):
    check_path("go")
    check_path("protoc-gen-go") 
  def create_makefile(self, uniquename):
    makefile = open("BUILD", "w")
    template = open(ROOT + "/template/BUILD.go.template", "r").read()
    makefile.write(template.format(uniquename=uniquename))
    makefile.close()        
  def generate_pb_files(self, uniquename):
    run_and_wait(["protoc", "-I", ".", "--go_out=plugins=grpc:.", uniquename + ".proto"])
    run_and_wait(["sed", "-i", "", "/SupportPackageIsVersion4/d", uniquename + ".pb.go"]) # TODO: reevaluate the life choices that led to this line
    genpath = ROOT + "/generated_go_pb_files/" + uniquename
    if os.path.exists(genpath):
      shutil.rmtree(genpath)
    os.mkdir(genpath)
    shutil.move(uniquename + ".pb.go", ROOT + "/generated_go_pb_files/" + uniquename + "/" + uniquename + ".pb.go")
    makefile = open("BUILD.pb", "w")
    template = open(ROOT + "/template/BUILD.go.pb.template", "r").read()
    makefile.write(template.format(uniquename=uniquename))
    makefile.close()
    shutil.move("BUILD.pb", ROOT + "/generated_go_pb_files/" + uniquename + "/BUILD")
  def do_prework(self, uniquename):
    print("go pre work")
    self.check_path()
    self.create_makefile(uniquename)
    self.generate_pb_files(uniquename)
  def generate_client(self, uniquename):
    print("go main work")
    run_and_wait(["protoc", "-I", ".", "--grpc_out=.", 
        "--plugin=protoc-gen-grpc=../../bazel-bin/go_generator", 
        uniquename + ".proto"])

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
argp.add_argument('-d', '--directory',
                  default="generated_probers",
                  help='directory to place generated files')
argp.add_argument('--no-rebuild', dest='rebuild',
                  action='store_false')
argp.set_defaults(rebuild=True)

args = argp.parse_args()

# ensure we run from the root dir of the repo
ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))
os.chdir(ROOT)
if not os.path.exists(args.directory):
  os.mkdir(args.directory)
GENERATED_DIR = os.path.join(ROOT, args.directory)

# validate the proto arg
if not args.proto.endswith(".proto"):
  print("proto file needs to be .proto")
  raise SystemExit(1)

check_path("protoc")
check_path("bazel")

languages = set(_LANGUAGES[l]
                for l in itertools.chain.from_iterable(
                      _LANGUAGES.iterkeys() if x == 'all' else [x]
                      for x in args.language))


# make the generators
if args.rebuild:
  if 'all' in args.language:
    run_and_wait(["bazel", "build", ":all"])
  else:
    for lang in languages:
      run_and_wait(["bazel", "build", ":" + lang.name() + "_generator"])

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

