
import resources
import grpc

def create_prober_channel(server_host, server_port, use_tls, 
    use_test_ca, server_host_override):
  target = '{}:{}'.format(server_host, server_port)
  call_credentials = None
  if use_tls:
    if use_test_ca:
      root_certificates = resources.test_root_certificates()
    else:
      root_certificates = None  # will load default roots.

    channel_credentials = grpc.ssl_channel_credentials(root_certificates)
    if call_credentials is not None:
      channel_credentials = grpc.composite_channel_credentials(
          channel_credentials, call_credentials)

    return grpc.secure_channel(target, channel_credentials, (
        ('grpc.ssl_target_name_override', server_host_override,),))
  else:
    return grpc.insecure_channel(target)
