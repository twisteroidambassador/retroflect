# retroflect
*Bust through UWP loopback restrictions by reflecting outbound
network traffic*

## Quick rundown

* **What it does**: reflect all network traffic targeted at "reflect
  address", so they appear to come from "reflect address" instead

* **What is it used for**: Circumvent Windows 8 and 10's restriction
  on UWP apps connecting to localhost

* **How to use it**:

  - Bind the server to `wildcard-address:server-port`

  - Run retroflect, specifying the reflect address

  - Direct the client to `reflect-address:server-port`, the client
    actually connects to your server

  - Profit!