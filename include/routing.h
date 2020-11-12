#pragma once

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <tuple>

struct Route
{
  std::array<uint8_t, 4> prefix;
  std::array<uint8_t, 4> next_hop;
  std::array<uint8_t, 4> mask;
  uint32_t interface;
};

class RoutingTable
{
private:
  RoutingTable() = default;
public:
  /*
   * Read from a file the routing information and save it in the routing table.
   * If the flag "has_error" is set, the readig is aborted.
   */
  static RoutingTable from_file(const std::string &filepath, bool &has_error);

  /*
   * Given an IP, return a tuple contaning the route and a flag signaling if the
   * route was found. If the flag is false, an empty route will be returned.
   */
  std::tuple<Route, bool> get_route(uint32_t ip) const;

private:
  std::vector<Route> routes;
};
