#pragma once

#include <cstdint>
#include <unistd.h>
#include <string.h>
#include <array>

#include "skel.h"

/*
 * The checksum function is from the code given in the source of lab 5.
 */
uint16_t checksum(void* vdata,size_t length);

/*
 * Given an IP as a string, convert it to an array of uint8_t.
 */
bool string_to_ip(std::string data, std::array<uint8_t, 4> &ip);
