#pragma once

#include <cstdint>
#include <map>
#include <array>

static constexpr uint8_t mac_broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

class ARP
{
public:
  /*
   * Receives a packet containing an ARP request as a parameter, inverts the source
   * destination and sets the MAC adress of the new source. Set the type in the arphdr to
   * ARP reply. Sends the modified packet.
   */
  static int handle_request(packet *p);
};

class ArpTable {
public:
  ArpTable() = default;

  /*
   * Receives an IP and an interface as parameters.
   * Creates a packet containing an ARP request from router to the IP given.
   * Sends the created packet.
   */
  static int send_request(uint8_t ip[4], int interface);

  /*
   * Receives a packet containing an ARP reply as a parameter. Adds to the ARP table
   * a new entry with the IP and MAC adress of the source.
   */
  int handle_reply(packet *p);

  /*
   * Receives an IP adress and returns the MAC adress.
   */
  std::tuple<std::array<uint8_t, 6>, bool> get_mac(const std::array<uint8_t, 4> &ip) const;

private:
  std::map<std::array<uint8_t, 4>, std::array<uint8_t, 6>> arp_table;
};
