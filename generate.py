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

# C++ specific makefile
_MAKEFILE_TEMPLATE = """HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CXXFLAGS += -std=c++11
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \\
					 -lgrpc++_reflection                                    \\
					 -lprotobuf -lpthread -ldl -lgflags
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \\
					 -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \\
					 -lprotobuf -lpthread -ldl -lgflags
endif

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

all: generated_client

generated_client: {uniquename}.pb.o {uniquename}.grpc.pb.o {uniquename}.grpc.client.pb.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.client.pb.cc
%.grpc.client.pb.cc: %.proto
	$(PROTOC) -I . --grpc_out=. --plugin=protoc-gen-grpc=../grpc_cpp_client_generator $<

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I . --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I . --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h generated_client
"""

# ensure we run from the root dir of the repo
ROOT = os.path.abspath(os.path.dirname(sys.argv[0]))
os.chdir(ROOT)

class CXXLanguage:
  def name(self):
    return "cpp"
  def create_makefile(self, uniquename):
    makefile = open("Makefile", "w")
    makefile.write(_MAKEFILE_TEMPLATE.format(uniquename=uniquename))
    makefile.close()
  def do_prework(self, uniquename):
    print("c++ pre work")
    self.create_makefile(uniquename)
  def generate_client(self, uniquename):
    print("c++ main work")
    proc = subprocess.Popen(args="make")
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
        "--plugin=protoc-gen-grpc=../grpc_go_client_generator", 
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
proc = subprocess.Popen(args="make")
proc.wait()

# generate the directories for each language
for lang in languages:
  os.chdir(ROOT) # back to root
  abspath = os.path.abspath(args.proto)
  uniquename = abspath.split("/")[-1][:-6]
  dirname = uniquename + "_" + lang.name()
  if os.path.exists(dirname):
    shutil.rmtree(dirname)
  os.mkdir(dirname)
  os.chdir(dirname)
  shutil.copyfile(abspath, os.getcwd() + "/" + uniquename + ".proto")
  lang.do_prework(uniquename)
  lang.generate_client(uniquename)

