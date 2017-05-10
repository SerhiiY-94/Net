#include "test_common.h"

#include <thread>
#include <chrono>

#include "../NAT_PMP.h"

void test_pmp() {

    {   // Should retrieve external ip first
        net::PMPSession s1(net::PMP_UDP, net::Address(127, 0, 0, 1, 5351), 30000, 30005);
        assert(s1.state() == net::PMPSession::RETRIEVE_EXTERNAL_IP);
    }

    {   // Retrieve external ip timeout
        net::PMPSession s1(net::PMP_UDP, net::Address(127, 0, 0, 1, 5351), 30000, 30005);
        s1.Update(64000);
        assert(s1.state() == net::PMPSession::IDLE_UNSUPPORTED);
        // should be idle forever
        s1.Update(100000000);
        assert(s1.state() == net::PMPSession::IDLE_UNSUPPORTED);
    }

    {   // Retrieve external ip unsupported version
        char recv_buf[128];

        net::UDPSocket fake_gateway;
        net::Address fake_gateway_address(127, 0, 0, 1, 30001);
        fake_gateway.Open(fake_gateway_address.port());

        net::PMPSession s1(net::PMP_UDP, fake_gateway_address, 30000, 30005);

        unsigned int time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::RETRIEVE_EXTERNAL_IP) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPExternalIPRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPUnsupportedVersionResponse resp(net::OP_EXTERNAL_IP_REQUEST);
                resp.set_res_code(2);
                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPUnsupportedVersionResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000 + 16);
        assert(s1.state() == net::PMPSession::IDLE_RETRIEVE_EXTERNAL_IP_ERROR);
    }

    {   // Retrieve external ip error
        char recv_buf[128];

        net::UDPSocket fake_gateway;
        fake_gateway.Open(30001);

        net::Address fake_gateway_address(127, 0, 0, 1, 30001);

        net::PMPSession s1(net::PMP_UDP, fake_gateway_address, 30000, 30005);

        unsigned int time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::RETRIEVE_EXTERNAL_IP) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPExternalIPRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPExternalIPRequest *req = (net::PMPExternalIPRequest *) recv_buf;
                assert(req->vers() == 0);
                assert(req->op() == 0);

                net::PMPExternalIPResponse resp;
                resp.set_ip(12345);
                resp.set_res_code(1);
                resp.set_time(1000);
                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPExternalIPResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000);
        assert(s1.state() == net::PMPSession::IDLE_RETRIEVE_EXTERNAL_IP_ERROR);
    }

    {   // Create port mapping timeout
        char recv_buf[128];

        net::UDPSocket fake_gateway;
        fake_gateway.Open(30001);

        net::Address fake_gateway_address(127, 0, 0, 1, 30001);

        net::PMPSession s1(net::PMP_UDP, fake_gateway_address, 30000, 30005);

        unsigned int time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::RETRIEVE_EXTERNAL_IP) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPExternalIPRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPExternalIPRequest *req = (net::PMPExternalIPRequest *) recv_buf;
                assert(req->vers() == 0);
                assert(req->op() == 0);

                net::PMPExternalIPResponse resp;
                resp.set_ip(12345);
                resp.set_res_code(0);
                resp.set_time(1000);
                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPExternalIPResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000);
        assert(s1.external_ip() == net::Address(12345, 0));
        assert(s1.state() == net::PMPSession::CREATE_PORT_MAPPING);
        assert(s1.time() == 1000);

        s1.Update(64000);

        assert(s1.state() == net::PMPSession::IDLE_UNSUPPORTED);
    }

    {   // Create port mapping error
        char recv_buf[128];

        net::UDPSocket fake_gateway;
        fake_gateway.Open(30001);

        net::Address fake_gateway_address(127, 0, 0, 1, 30001);

        net::PMPSession s1(net::PMP_UDP, fake_gateway_address, 30000, 30005);

        unsigned int time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::RETRIEVE_EXTERNAL_IP) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPExternalIPRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPExternalIPRequest *req = (net::PMPExternalIPRequest *) recv_buf;
                assert(req->vers() == 0);
                assert(req->op() == 0);

                net::PMPExternalIPResponse resp;
                resp.set_ip(12345);
                resp.set_res_code(0);
                resp.set_time(1000);
                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPExternalIPResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::CREATE_PORT_MAPPING) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPMappingRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPMappingRequest *req = (net::PMPMappingRequest *) recv_buf;
                assert(req->vers() == 0);
                assert(req->op() == 1);
                assert(req->internal_port() == 30000);
                assert(req->external_port() == 30005);
                assert(req->lifetime() == 7200);

                net::PMPMappingResponse resp(net::PMP_UDP);
                resp.set_res_code(net::PMP_RES_FAILLURE);

                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPMappingResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000);
        assert(s1.state() == net::PMPSession::IDLE_CREATE_PORT_MAPPING_ERROR);
    }

    {   // Create port mapping success
        char recv_buf[128];

        net::UDPSocket fake_gateway;
        fake_gateway.Open(30001);

        net::Address fake_gateway_address(127, 0, 0, 1, 30001);

        net::PMPSession s1(net::PMP_UDP, fake_gateway_address, 30000, 30005);

        unsigned int time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::RETRIEVE_EXTERNAL_IP) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPExternalIPRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPExternalIPRequest *req = (net::PMPExternalIPRequest *) recv_buf;
                assert(req->vers() == 0);
                assert(req->op() == 0);

                net::PMPExternalIPResponse resp;
                resp.set_ip(12345);
                resp.set_res_code(0);
                resp.set_time(1000);
                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPExternalIPResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::CREATE_PORT_MAPPING) {
            net::Address sender;
            if (fake_gateway.Receive(sender, recv_buf, sizeof(recv_buf)) == sizeof(net::PMPMappingRequest) &&
                sender == net::Address(127, 0, 0, 1, s1.local_addr().port())) {
                net::PMPMappingRequest *req = (net::PMPMappingRequest *) recv_buf;
                assert(req->vers() == 0);
                assert(req->op() == 1);
                assert(req->internal_port() == 30000);
                assert(req->external_port() == 30005);
                assert(req->lifetime() == 7200);

                net::PMPMappingResponse resp(net::PMP_UDP);
                resp.set_res_code(0);
                resp.set_internal_port(30000);
                resp.set_external_port(30005);
                resp.set_time(1000);
                resp.set_lifetime(7200);

                fake_gateway.Send(s1.local_addr(), &resp, sizeof(net::PMPMappingResponse));
            }

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000);
        assert(s1.state() == net::PMPSession::IDLE_MAPPED);

        // simulate nat restart

        time_acc = 0;

        while (time_acc < 1000 && s1.state() == net::PMPSession::IDLE_MAPPED) {
            net::PMPExternalIPResponse resp;
            resp.set_ip(12345);
            resp.set_res_code(0);
            resp.set_time(0);

            fake_gateway.Send(net::Address(224, 0, 0, 1, 5350), &resp, sizeof(net::PMPExternalIPResponse));

            s1.Update(16);

            time_acc += 16;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        assert(time_acc < 1000);
        assert(s1.state() == net::PMPSession::CREATE_PORT_MAPPING);
    }
}
