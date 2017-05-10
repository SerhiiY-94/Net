#include "test_common.h"

#include "../Address.h"

void test_address() {

    {   net::Address addr;
        assert(addr.a() == 0);
        assert(addr.b() == 0);
        assert(addr.c() == 0);
        assert(addr.d() == 0);
        assert(addr.port() == 0);
        assert(addr.address() == 0);
    }

    {
        const unsigned char a = 100;
        const unsigned char b = 110;
        const unsigned char c = 50;
        const unsigned char d = 12;
        const unsigned short port = 10000;
        net::Address address(a, b, c, d, port);
        assert(a == address.a());
        assert(b == address.b());
        assert(c == address.c());
        assert(d == address.d());
        assert(port == address.port());
    }

    {
        net::Address x(100, 110, 0, 1, 50000);
        net::Address y(101, 210, 6, 5, 50002);
        assert(x != y);
        assert(y == y);
        assert(x == x);
    }

    {
        net::Address x(100, 110, 0, 1, 50000);
        std::string s = x.str();
        assert(s == "100.110.0.1:50000");
    }
}