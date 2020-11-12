#include "routing.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

RoutingTable RoutingTable::from_file(const std::string &filepath, bool &has_error)
{
  RoutingTable rt;

  std::ifstream fin(filepath);
  std::string line;
  Route route;

  has_error = false;

  // Read for as long as the file has not ended and there were no errors.
  while(std::getline(fin, line) && !has_error) {
    std::istringstream iss(line);
    std::string a;

    // Read the prefix from the file.
    iss >> a;
    if (!string_to_ip(a, route.prefix)) {
      has_error = true;
    }

    // Read the next_hop from the file.
    iss >> a;
    if (!string_to_ip(a, route.next_hop)) {
      has_error = true;
    }

    // Read the mask from the file.
    iss >> a;
    if (!string_to_ip(a, route.mask)) {
      has_error = true;
    }

    // Read the interface from the file.
    iss >> a;
    route.interface = std::atoi(a.c_str());

    // Add the new entry to the routing table.
    rt.routes.push_back(route);
  }

  // Sort the routes
  std::sort(rt.routes.begin(), rt.routes.end(), [](const Route &a, const Route &b) {
    if(a.prefix == b.prefix) {
      return a.mask < b.mask;
    }

    return a.prefix < b.prefix;
  });

  return rt;
}

std::tuple<Route, bool> RoutingTable::get_route(uint32_t ip) const
{
  std::array<uint8_t, 4> dest_ip_arr;
  memcpy(dest_ip_arr.data(), &ip, 4);

  // Binary search to find the best route in O(nlogn).
  int left = 0;
  int right = routes.size();

  while (left < right) {
    int middle = (left + right) / 2;

    if (routes[middle].prefix > dest_ip_arr) {
      right = middle;
    } else {
      left = middle + 1;
    }
  }

  const auto &best_route = routes[right - 1];
  uint32_t int_mask = *((uint32_t *) best_route.mask.data());
  uint32_t int_prefix = *((uint32_t *) best_route.prefix.data());

  if (right == 0 || (int_mask & ip) != int_prefix) {
    return std::make_tuple(Route{}, false);
  }

  return std::make_tuple(routes[right - 1], true);
}
