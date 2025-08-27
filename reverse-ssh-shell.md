# Reverse SSH Shell

## Remote Port Forwarding

```
# Setup

[ PWNED SERVER ]


[ GCP GATEWAY VM (35.196.145.50) ]


[ LAPTOP ]


```

- `ssh -R [remote_addr:]remote_port:local_addr:local_port [user@]gateway_addr`
  - `-R`: remote port forwarding
  - `remote_addr:remote_port`: the interface/address on the gateway where the forwarded port should appear
  - `local_addr:local_port`: service on pwned server to expose
  - `gateway_addr`: public-facing SSH server you connect to from laptop

- when someone connects to `remote_addr:remote_port` _on the gateway_, traffic is tunneled over SSH to pwned server, then delivered to `local_addr:local_port`
- must turn on `GatewayPorts yes` for SSH daemon on gateway to bind to all interfaces (0.0.0.0)

- port forwarding: taking traffic that arrives at one port on one machine, securely forwarding it thorugh an SSH tunnel to another machine/port
- a port forwarding tunnel just carries raw TCP packages over an encrypted SSH channel

## Reverse SSH Shell
