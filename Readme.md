# Homework 1: Router
Grigore Iulia-Anita, 322CA

For this homework, I organized my code by functionality: ARP, ICMP and routing table, each
having a corresponding header and source file.

## ARP

To implement the ARP protocol, I created 3 functions: `ARP::handle_request`,
`ArpTable::send_request` and `ArpTable::handle_reply`. Additionally, a getter that receives
an IP and returns a tuple of the corresponding MAC address and a bool, signaling whether the
address had been found.

### `ARP::handle_request`

When a packet containing an ARP request arrives at the router, this function changes the MAC
address of destination from broadcast to the one of the router, interchanges the source with the
destination in the ethernet header and in the ARP header and sends the packet, now containing
an ARP reply, back.

### `ArpTable::send_request`

To forward a packet, the MAC and IP address need to be known by the router. This pair is saved into
an arp table when an ARP reply is received. If it's not found in the table, the router sends an ARP
request. To do that, this function creates a packet from scratch, populates the fields of the
`ethhdr`, `arphdr` and `arp_body` (a structure containing the information about the source and
destination of an ARP packet), computes the length of the packet and sends it to broadcast.

### `ArpTable::handle_reply`

If the router receives an ARP reply, extract the IP and MAC addresses and add a new entry
containing them into the arp table.

### `ArpTable::get_mac`

Given an IP address, returns the MAC address from the arp table.

## ICMP

There are two functions implementing different scenarios of ICMP: `IcmpHandler::send_errors`
and `IcmpHandler::handle_echo`.

### `IcmpHandler::send_errors`

This function is generic, sending any error identified by type and code to the given destination.
In this homework, it's used for nothing but `Time exceeded` and `Host Unreachable` errors.

### `IcmpHandler::handle_echo`

Given an ECHO request packet, sets the source MAC address to the one of the router, swaps
the source and destination. Updates the `ttl` and recomputes the `checksum`. Sends the modified
packet as an echo reply.

## Routing Process

`RoutingTable::from_file` is the function that parses the routing table file, sorts the table for faster
lookup and returns a newly constructed RoutingTable instance.

`RoutingTable::get_route` finds the best route using a customized version of a binary search algorithm. This
variant of the algorithm is made to find the rightmost value in case of duplicates. Having the
rightmost value in the routing table returned, together with having the routing table sorted the
way it is, ensure this function will indeed return the best possible route. The time complexity
is given by the binary search algorithm, O(log n).

`process_packet` returns true if a packet was processed, false otherwise. Processing a
packet starts with checking for errors (time exceeded and host unreachable). The function is
responsible for finding the best route for an incoming packet. If the identified route has an
unknown MAC address, address resolution is done by placing the packet in a queue, issuing an ARP
request and waiting for an ARP reply.

The entry point of the application consists of an infinite loop waiting for packets. Each tick
extracts the Ethernet header and does triage for the incoming packet.

ARP packet:
  - if it's a request, handle it using the function `ARP::handle_request`.
  - if it's a reply, handle it using `ArpTable::handle_reply`. After the reply has been processed, the
  application performs a lookup for packets that can now be routed from the waiting queue.

IP packet:
  - if the destination of the packet is not the router, forward it.
  - if the protocol is ICMP, handle ECHO requests.
