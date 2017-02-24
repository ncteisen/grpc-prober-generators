/*
 *
 * Copyright 2014, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

package interop

import (
  "net"
  "strconv"

  "github.com/golang/glog"
  "google.golang.org/grpc"
  "google.golang.org/grpc/credentials"
)

var (
  testCAFile = "testdata/ca.pem"
)

// Creates and returns a grpc Channel using the given flags.
func CreateProberChannel(serverHost *string, serverPort *int, 
    tlsServerName *string, useTLS *bool, testCA *bool) (*grpc.ClientConn) {
  serverAddr := net.JoinHostPort(*serverHost, strconv.Itoa(*serverPort))
  var opts []grpc.DialOption
  if *useTLS {
    var sn string
    if *tlsServerName != "" {
      sn = *tlsServerName
    }
    var creds credentials.TransportCredentials
    if *testCA {
      var err error
      creds, err = credentials.NewClientTLSFromFile(testCAFile, sn)
      if err != nil {
        glog.Fatalf("Failed to create TLS credentials %v", err)
      }
    } else {
      creds = credentials.NewClientTLSFromCert(nil, sn)
    }
    opts = append(opts, grpc.WithTransportCredentials(creds))
  } else {
    opts = append(opts, grpc.WithInsecure())
  }
  channel, err := grpc.Dial(serverAddr, opts...)
  if err != nil {
    glog.Fatalf("Fail to dial: %v", err)
  }
  return channel
}
