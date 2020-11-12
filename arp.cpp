#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <cstdint>
#include <arpa/inet.h>

#include <algorithm>
#include <iterator>

#include "skel.h"
#include "arp.h"

#include <iostream>
#include <iomanip>

struct arp_body
{
  uint8_t	ar_sha[6];
	uint8_t	ar_spa[4];
	uint8_t	ar_tha[6];
	uint8_t	ar_tpa[4];
};

int ARP::handle_request(packet *p)
{
  ethhdr *eth = (ethhdr*) p->payload;
  arphdr *arp = (arphdr*) (p->payload + sizeof(ethhdr));
  arp_body *arp_b = (arp_body*) (p->payload + sizeof(ethhdr) + sizeof(arphdr));

  // Interchanges the MAC addresses of the sorce and destination. Sets the address of
  // the source to the MAC of the router.
  uint8_t temp_mac[6];
  memcpy(temp_mac, eth->h_source, 6);
  get_interface_mac(p->interface, eth->h_source);
  memcpy(eth->h_dest, temp_mac, 6);

  arp->ar_op = htons(ARPOP_REPLY);
  memcpy(arp_b->ar_tha, eth->h_dest, 6);
  memcpy(arp_b->ar_sha, eth->h_source, 6);

  // Interchanges the IP addresses of the source and destination.
  uint8_t temp_ip[4];
  memcpy(temp_ip, arp_b->ar_spa, 4);
  memcpy(arp_b->ar_spa, arp_b->ar_tpa, 4);
  memcpy(arp_b->ar_tpa, temp_ip, 4);

  return send_packet(p->interface, p);
}


int ArpTable::send_request(uint8_t ip[4], int interface)
{
  // Creates an ARP packet.
  packet p = {0};
  ethhdr *eth = (ethhdr*) p.payload;
  arphdr *arp = (arphdr*) (p.payload + sizeof(ethhdr));
  arp_body *arp_b = (arp_body*) (p.payload + sizeof(ethhdr) + sizeof(arphdr));

  eth->h_proto = htons(ETH_P_ARP);

  get_interface_mac(interface, eth->h_source);
  memcpy(eth->h_dest, mac_broadcast, 6);

  arp->ar_hrd = htons(ARPHRD_ETHER);
  arp->ar_pro = htons(ETH_P_IP);
  arp->ar_hln = 6;
  arp->ar_pln = 4;
  arp->ar_op = htons(ARPOP_REQUEST);

  // Set the IP and MAC addresses of the source to the ones of the router. Set the IP
  // address to the parameter given and the MAC address to the broadcast.
  get_interface_mac(interface, arp_b->ar_sha);
  inet_pton(AF_INET, get_interface_ip(interface), arp_b->ar_spa);
  memcpy(arp_b->ar_tha, mac_broadcast, 6);
  memcpy(arp_b->ar_tpa, ip, 4);

  p.interface = interface;
  p.len = sizeof(ethhdr) + sizeof(arphdr) + sizeof(arp_body);

  send_packet(interface, &p);
}

int ArpTable::handle_reply(packet *p)
{
  ethhdr *eth = (ethhdr*) p->payload;
  arphdr *arp = (arphdr*) (p->payload + sizeof(ethhdr));
  arp_body *arp_b = (arp_body*) (p->payload + sizeof(ethhdr) + sizeof(arphdr));

  // Extract the IP and MAC addresses from the packet.
  std::array<uint8_t, 4> ip;
  std::copy_n(std::begin(arp_b->ar_spa), 4, ip.begin());

  std::array<uint8_t, 6> mac;
  std::copy_n(std::begin(arp_b->ar_sha), 6, mac.begin());

  // Add the entry to the ARP table.
  arp_table.emplace(ip, mac);
}

std::tuple<std::array<uint8_t, 6>, bool> ArpTable::get_mac(
  const std::array<uint8_t, 4> &ip) const
{
  auto it = arp_table.find(ip);
  // If the iterator points to the end of the table, the entry does not exist.
  // Return an empty array with the flag set to false (not found).
  if (it == arp_table.end()) {
    return std::make_tuple(std::array<uint8_t, 6>{}, false);
  }

  return std::make_tuple(it->second, true);
}
