#include "test_common.h"

#include <thread>
#include <chrono>

#include "../NAT_PCP.h"

void test_pcp() {

    {   // Request read/write
        char buf[128];

        net::PCPNonce nonce;
        net::GenPCPNonce(&nonce, sizeof(net::PCPNonce));

        net::Address client_address(127, 1, 2, 3, 0);
        net::Address external_address(77, 4, 5, 6, 0);
        net::Address remote_address(196, 4, 5, 6, 0);

        {
            net::PCPRequest req;
            req.MakeAnnounceRequest(client_address);

            int sz = req.Write(buf, sizeof(buf));
            assert(sz != -1);

            net::PCPRequest req1;

            assert(req1.Read(buf, sz));
            assert(req1.opcode() == net::OP_ANNOUNCE);
            assert(req1.lifetime() == 0);
            assert(req1.client_address().address() == client_address.address());
        }

        {
            net::PCPRequest req;
            req.MakeMapRequest(net::PCP_UDP, 12345, 17891, 7200, client_address, nonce);

            int sz = req.Write(buf, sizeof(buf));
            assert(sz != -1);

            net::PCPRequest req1;

            assert(req1.Read(buf, sz));
            assert(req1.opcode() == net::OP_MAP);
            assert(req1.internal_port() == 12345);
            assert(req1.external_port() == 17891);
            assert(req1.lifetime() == 7200);
            assert(req1.client_address().address() == client_address.address());
            assert(req1.nonce() == nonce);
            assert(req1.proto() == net::PCP_UDP);
            assert(req1.external_address() == net::Address());
        }

        {
            net::PCPRequest req;
            req.MakePeerRequest(net::PCP_UDP, 12345, 17891, 7200, external_address, 55765, remote_address, nonce);

            int sz = req.Write(buf, sizeof(buf));
            assert(sz != -1);

            net::PCPRequest req1;

            assert(req1.Read(buf, sz));
            assert(req1.opcode() == net::OP_PEER);
            assert(req1.internal_port() == 12345);
            assert(req1.external_port() == 17891);
            assert(req1.lifetime() == 7200);
            assert(req1.external_address() == external_address);
            assert(req1.remote_port() == 55765);
            assert(req1.proto() == net::PCP_UDP);
            assert(req1.remote_address() == remote_address);
            assert(req1.nonce() == nonce);
        }
    }

    {   // Response read/write
        char buf[128];

        net::PCPNonce nonce;
        net::GenPCPNonce(&nonce, sizeof(net::PCPNonce));

        net::Address external_address(77, 78, 79, 3, 0), remote_address(111, 12, 14, 3, 0);

        {
            net::PCPResponse resp;
            resp.MakeAnnounceResponse(net::PCP_RES_SUCCESS);

            int sz = resp.Write(buf, sizeof(buf));
            assert(sz != -1);

            net::PCPResponse resp1;

            assert(resp1.Read(buf, sz));
            assert(resp1.opcode() == net::OP_ANNOUNCE);
            assert(resp1.res_code() == net::PCP_RES_SUCCESS);
            assert(resp1.lifetime() == 0);
        }

        {
            net::PCPResponse resp;
            resp.MakeMapResponse(net::PCP_UDP, net::PCP_RES_SUCCESS, 12345, 17891, 7200, 120, external_address, nonce);

            int sz = resp.Write(buf, sizeof(buf));
            assert(sz != -1);

            net::PCPResponse resp1;

            assert(resp1.Read(buf, sz));
            assert(resp1.opcode() == net::OP_MAP);
            assert(resp1.res_code() == net::PCP_RES_SUCCESS);
            assert(resp1.lifetime() == 7200);
            assert(resp1.time() == 120);
            assert(resp1.external_address().address() == external_address.address());
            assert(resp1.nonce() == nonce);
        }

        {
            net::PCPResponse resp;
            resp.MakePeerResponse(net::PCP_UDP, net::PCP_RES_SUCCESS, 12345, 17891, 7200, 120, external_address, 61478,
                                  remote_address, nonce);

            int sz = resp.Write(buf, sizeof(buf));
            assert(sz != -1);

            net::PCPResponse resp1;

            assert(resp1.Read(buf, sz));
            assert(resp1.opcode() == net::OP_PEER);
            assert(resp1.res_code() == net::PCP_RES_SUCCESS);
            assert(resp1.lifetime() == 7200);
            assert(resp1.time() == 120);
            assert(resp1.external_address().address() == external_address.address());
            assert(resp1.nonce() == nonce);
        }
    }

    {   // Retransmit
        char buf[128];

        net::UDPSocket fake_pcp_srv;
        fake_pcp_srv.Open(30001);

        net::Address fake_pcp_address(127, 0, 0, 1, 30001);

        net::PCPSession ses(net::PCP_UDP, fake_pcp_address, 30000, 30001, 7200);
        assert(ses.state() == net::PCPSession::REQUEST_MAPPING);

        ses.Update(16);

        net::Address sender;
        assert(fake_pcp_srv.Receive(sender, buf, sizeof(buf)));

        ses.Update(16);
        assert(!fake_pcp_srv.Receive(sender, buf, sizeof(buf)));

        ses.Update(4000);
        assert(fake_pcp_srv.Receive(sender, buf, sizeof(buf)));

        ses.Update(16);
        assert(!fake_pcp_srv.Receive(sender, buf, sizeof(buf)));
    }

    {
        char recv_buf[128];
        int size = 0;

        net::UDPSocket pcp_server;
        pcp_server.Open(30001);

        net::Address pcp_server_address(127, 0, 0, 1, 30001);
        net::Address external_address(77, 22, 44, 55, 0);

        net::PCPSession s1(net::PCP_UDP, pcp_server_address, 30000, 30005);

        unsigned int time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PCPSession::REQUEST_MAPPING) {
            net::Address sender;
            net::PCPRequest req;
            if ((size = pcp_server.Receive(sender, recv_buf, sizeof(recv_buf))) && req.Read(recv_buf, size)) {
                net::PCPResponse resp;
                resp.MakeMapResponse(net::PCP_UDP, net::PCP_RES_SUCCESS, 30000, 30001, 7200, 100, external_address, req.nonce());

                size = resp.Write(recv_buf, sizeof(recv_buf));
                pcp_server.Send(s1.local_addr(), &resp, size);
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000);
        assert(s1.state() == net::PCPSession::IDLE_MAPPED);
    }
}
