#pragma once

#include "VarContainer.h"

namespace net {
	net::Packet CompressLZO(const net::Packet &pack);
	net::Packet DecompressLZO(const net::Packet &pack);
}
