# Copyright 2016, Google Inc.
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

import grpc
import argparse
import os

import pkg_resources

_ROOT_CERTIFICATES_RESOURCE_PATH = 'credentials/ca.pem'

def _args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--server_host',
        help='the host to which to connect',
        type=str,
        default="localhost")
    parser.add_argument(
        '--server_port', 
        help='the port to which to connect',
        default=8080,
        type=int)
    parser.add_argument(
        '--use_tls',
        help='require a secure connection',
        default=False,
        type=parse_bool)
    parser.add_argument(
        '--use_test_ca',
        help='replace platform root CAs with ca.pem',
        default=False,
        type=parse_bool)
    parser.add_argument(
        '--server_host_override',
        default="foo.test.google.fr",
        help='the server host to which to claim to connect',
        type=str)
    return parser.parse_args()

def test_root_certificates():
    return pkg_resources.resource_string(__name__,
                                         _ROOT_CERTIFICATES_RESOURCE_PATH)

def parse_bool(value):
    if value == 'true':
        return True
    if value == 'false':
        return False
    raise argparse.ArgumentTypeError('Only true/false allowed')

def create_prober_channel():
  args = _args()
  print(args)
  target = '{}:{}'.format(args.server_host, args.server_port)
  call_credentials = None
  if args.use_tls:
    if args.use_test_ca:
      root_certificates = resources.test_root_certificates()
    else:
      root_certificates = None  # will load default roots.

    channel_credentials = grpc.ssl_channel_credentials(root_certificates)
    if call_credentials is not None:
      channel_credentials = grpc.composite_channel_credentials(
          channel_credentials, call_credentials)

    return grpc.secure_channel(target, channel_credentials, (
        ('grpc.ssl_target_name_override', args.server_host_override,),))
  else:
    return grpc.insecure_channel(target)
