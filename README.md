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


## System Requirements

Retroflect runs on Windows. It requires Python 3.6.x and
[pydivert](https://pypi.org/project/pydivert/).

Pydivert doesn't seem to support Python 3.7 yet, and retroflect uses
features introduced in 3.6, so that's the only version supported at the
time.

(That said, it shouldn't be difficult to backport retroflect.
Replace f-strings with `format()` and it should run on 3.5. Remove
type annotations and it should run on 3.4.)


## Features

Retroflect reflects outgoing network traffic back at the sender.
When retroflect is running on a system with one or more
"reflect addresses" configured, any network traffic sent by the
system towards a reflect address are filtered, modified and
re-injected, so it looks like the traffic is coming from the reflect
address towards the system instead.

Additionally, retroflect can also be configured with zero or more
"shield ports". When configured, retroflect acts like a firewall and
drops any incoming traffic destined for these ports.


## Usage

Retroflect is designed for one use case: running a server on localhost
for clients on the same system, while disguising it as a server on
another system. This neatly sidesteps the UWP loopback restriction on
Windows 8 and up.

By default, UWP apps ("universal apps", "modern apps", most
Store apps) are not allowed to connect to localhost over network.
Running a proxy server on localhost and set it as the system proxy?
All UWP apps will lose Internet access, even the Edge browser (unless
you go to about:flags and tick a box) and Microsoft Store. Retroflect
helps you use a localhost system proxy, without needing to [grant
loopback exemption to every single UWP app](https://msdn.microsoft.com/en-us/library/windows/apps/hh780593.aspx)
(and then remember to grant it to every new app installed, etc etc.)

To use retroflect for this purpose, follow these steps:

* Choose one or more reflect address

* Bind the localhost server to the wildcard address

* Run retroflect, specifying the reflect address and optionally
  shielding the server port

* Direct client software to connect to `reflect-address:server-port`


### Choose reflect address(es)

A reflect address must meet the following requirements:

**It must not be one of the addresses associated with the system.**
Windows is smart enough to figure out that the destination is the
same machine, and will correctly flag such traffic as loopback.

**It must not be an address you actually want to connect to.**
Because all traffic towards the reflect address will be, naturally,
reflected, and will never reach the destination.

**It must not be in the same subnet as any of the system's IP
addresses.** If so, Windows will attempt to discover the
MAC address associated with the reflect address by ARP (IPv4) or
neighbor discovery (IPv6), and when nobody answers, fail to send the
traffic.

**There must be a route to it.**
Usually this will be the default route, but do make sure you have a
default route. As an example, if a system only has IPv6 Internet
connectivity, there probably is no IPv4 default route, and an IPv4
reflect address won't work.


### Bind server to wildcard address

The server running on the system must be bound to the wildcard
address (`0.0.0.0` or `::`) in order to receive reflected traffic,
which will appear to come from the reflect address.

Make sure Windows Firewall is not blocking the server.

Binding to the wildcard address does mean that other computers on the
network may connect to the server as well. If this is not desired,
you can specify the server's port(s) as shield ports when running
retroflect, which will drop incoming traffic towards those ports and
protect the server.


### Running retroflect

Retroflect must be run with Administrator privileges. Open an
Administrator cmd prompt or powershell prompt and run:

    retroflect.py [-s <shield-port>] <reflect-address>

Normally there won't be any console output. For help about command line arguments, run `retroflect.py -h`.

Once it's running, any client software making a connection to
`reflect-address:server-port` will actually connect to the server
running on localhost. For the server, it looks like the client is
connecting from the reflect address. Neither the client nor the server
will realize that the other party is running on the same system.
Mission accomplished!
