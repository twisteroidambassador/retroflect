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


## Theory

### Design Goal

Retroflect reflects outgoing network traffic back at the sender.
When retroflect is running on a system with one or more
"reflect addresses" configured, any network traffic sent by the
system towards a reflect address are filtered, modified and
re-injected, so it looks like the traffic is coming from the reflect
address towards the system instead.

Additionally, retroflect can also be configured with zero or more
"shield ports". When configured, retroflect acts like a firewall and
drops any incoming traffic destined for these ports.


### Use Case

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


### Choosing reflect address(es)

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


### Binding server to wildcard address

The server running on the system must be bound to the wildcard
address (`0.0.0.0` or `::`) in order to receive reflected traffic,
which will appear to come from the reflect address.

Binding to the wildcard address does mean that other computers on the
network may connect to the server as well. If this is not desired,
you can specify the server's port(s) as shield ports when running
retroflect, which will drop incoming traffic towards those ports and
protect the server.


## Implementation

This repository contains two implementations of retroflect: a Python
version, and a C++ version. The C++ version is recommended, since it
uses a more up-to-date version of WinDivert, and should have better
performance.


### System Requirements

#### C++

Using the C++ version of retroflect downloaded from the
[Releases page](https://github.com/twisteroidambassador/retroflect/releases)
does not require any 3rd-party dependency, since the required WinDivert
drivers are bundled in.

Building the C++ version of retroflect requires Visual Studio and
[WinDivert 1.4.3](https://reqrypt.org/windivert.html). I used Visual
Studio 2017 Community to develop and build the code; other versions
may also work. See `build.md` for details.


#### Python

The Python version of retroflect requires Python 3.6.x and
[pydivert](https://pypi.org/project/pydivert/).
Pydivert doesn't seem to support Python 3.7 yet, and retroflect uses
features introduced in 3.6, so that's the only version supported at the
time.
(That said, it shouldn't be difficult to backport retroflect.
Replace f-strings with `format()` and it should run on 3.5. Remove
type annotations and it should run on 3.4.)
Pydivert bundles WinDivert 1.3, so no separate WinDivert is
required.


### Running retroflect

#### C++

Retroflect must run with Administrator privileges. It will request
UAC elevation if required. Just run:

    retroflect.exe <reflect-address-or-shield-port> ...

Multiple reflect addresses and shield ports may be specified in any
order.

When starting, the specified reflect addresses and shield ports are
printed to the console, then "Reflection in progress..." is printed.
`Ctrl+C` can be used to stop retroflect.


#### Python

Retroflect must be run with Administrator privileges. Open an
Administrator cmd prompt or powershell prompt and run:

    retroflect.py [-s <shield-port>] <reflect-address>

Normally there won't be any console output. For help about command line
arguments, run `retroflect.py -h`.

`Ctrl+C` may not work to stop retroflect. Try `Break` or `Ctrl+Break`.

#### ---

Once retroflect is running, any client software making a connection to
`reflect-address:server-port` will actually connect to the server
running on localhost. For the server, it looks like the client is
connecting from the reflect address. Neither the client nor the server
will realize that the other party is running on the same system.
Mission accomplished!


### Known issues

The C++ and Python versions of retroflect currently have different
behavior. The C++ version only reflects TCP traffic towards
reflect addresses, and only shields incoming TCP traffic towards shield
ports. The Python version reflects all network traffic towards reflect
addresses, but also only shields incoming TCP traffic.

Also, with the Python version,
when a server port is shielded any attempt to access it via the
localhost address (`127.0.0.1:server-port`, `[::1]:server-port`, etc.)
will also fail. This is because pydivert still uses WinDivert 1.3,
and that version does not handle loopback traffic well. The C++ version
does allow loopback traffic towards shield ports.
