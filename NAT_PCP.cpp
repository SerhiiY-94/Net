#include "NAT_PCP.h"

#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
#endif
#if defined(__linux__)
    #include <arpa/inet.h>
#endif

bool Net::GenPCPNonce(void *buf, int len) {
    if (len < 12) return false;

    int32_t *p = (int32_t *)buf;
    p[0] = rand();
    p[1] = rand();
    p[2] = rand();

    return true;
}

/*************** PCPRequest ***************/

bool Net::PCPRequest::Read(const void *buf, int size) {
    uint8_t *p = (uint8_t *)buf;

    if (size < 24) return false;

    if (p[0] != 2 || (p[1] & (1 << 7))) return false; // 0b10000000

    opcode_ = (PCPOpCode)(p[1] & ((1 << 7) - 1)); // 0b01111111

    lifetime_ = ntohl(*(uint32_t*)(&p[4]));

    client_address_ = Net::Address(p[20], p[21], p[22], p[23], 0);

    p = &p[24];
    if (opcode_ == OP_ANNOUNCE) {
        return true;
    } else if (opcode_ == OP_MAP) {
        if (size < 24 + 36) return false;

        memcpy(&nonce_, &p[0], sizeof(PCPNonce));
        proto_ = (PCPProto) p[12];

        internal_port_ = ntohs(*(uint16_t *) (&p[16]));
        external_port_ = ntohs(*(uint16_t *) (&p[18]));

        external_address_ = Net::Address(p[32], p[33], p[34], p[35], 0);

        return true;
    } else if (opcode_ == OP_PEER) {
        if (size < 24 + 56) return false;

        memcpy(&nonce_, &p[0], sizeof(PCPNonce));
        proto_ = (PCPProto) p[12];

        internal_port_ = ntohs(*(uint16_t *) (&p[16]));
        external_port_ = ntohs(*(uint16_t *) (&p[18]));

        external_address_ = Net::Address(p[32], p[33], p[34], p[35], 0);

        remote_port_ = ntohs(*(uint16_t *) (&p[36]));
        remote_address_ = Net::Address(p[52], p[53], p[54], p[55], 0);

        return true;
    } else {
        return false;
    }
}

int Net::PCPRequest::Write(void *buf, int size) const {
    uint8_t *p = (uint8_t *)buf;

    if (opcode_ == OP_NONE || size < 24) return -1;

    // version
    p[0] = 2;

    // request
    p[1] = 0;

    // opcode
    p[1] |= opcode_ & 0xFF;

    // reserved
    p[2] = 0;
    p[3] = 0;

    // filetime
    *(uint32_t*)(&p[4]) = htonl(lifetime_);

    // ipv4 mapped to ipv6
    p[8] = p[9] = p[10] = p[11] = 0;
    p[12] = p[13] = p[14] = p[15] = 0;
    p[16] = p[17] = 0;
    p[18] = p[19] = 0xFF;
    p[20] = client_address_.a();
    p[21] = client_address_.b();
    p[22] = client_address_.c();
    p[23] = client_address_.d();

    p = &p[24];
    if (opcode_ == OP_ANNOUNCE) {
        return 24;
    } else if (opcode_ == OP_MAP) {
        if (size < 24 + 36) return -1;

        memcpy(&p[0], &nonce_, sizeof(PCPNonce));
        p[12] = proto_;
        p[13] = p[14] = p[15] = 0;

        *(uint16_t *) (&p[16]) = htons(internal_port_);
        *(uint16_t *) (&p[18]) = htons(external_port_);

        // ipv4 mapped to ipv6
        p[20] = p[21] = p[22] = p[23] = 0;
        p[24] = p[25] = p[26] = p[27] = 0;
        p[28] = p[29] = 0;
        p[30] = p[31] = 0xFF;
        p[32] = external_address_.a();
        p[33] = external_address_.b();
        p[34] = external_address_.c();
        p[35] = external_address_.d();

        return 24 + 36;
    } else if (opcode_ == OP_PEER) {
        if (size < 24 + 56) return -1;

        memcpy(&p[0], &nonce_, sizeof(PCPNonce));
        p[12] = proto_;
        p[13] = p[14] = p[15] = 0;

        *(uint16_t *) (&p[16]) = htons(internal_port_);
        *(uint16_t *) (&p[18]) = htons(external_port_);

        // ipv4 mapped to ipv6
        p[20] = p[21] = p[22] = p[23] = 0;
        p[24] = p[25] = p[26] = p[27] = 0;
        p[28] = p[29] = 0;
        p[30] = p[31] = 0xFF;
        p[32] = external_address_.a();
        p[33] = external_address_.b();
        p[34] = external_address_.c();
        p[35] = external_address_.d();

        *(uint16_t *) (&p[36]) = htons(remote_port_);
        p[38] = p[39] = 0;

        // ipv4 mapped to ipv6
        p[40] = p[41] = p[42] = p[43] = 0;
        p[44] = p[45] = p[46] = p[47] = 0;
        p[48] = p[49] = 0;
        p[50] = p[51] = 0xFF;
        p[52] = remote_address_.a();
        p[53] = remote_address_.b();
        p[54] = remote_address_.c();
        p[55] = remote_address_.d();

        return 24 + 56;
    } else {
        return -1;
    }
}

/*************** PCPResponse ***************/

bool Net::PCPResponse::Read(const void *buf, int size) {
    uint8_t *p = (uint8_t *)buf;

    if (size < 24) return false;

    if (p[0] != 2 || !(p[1] & (1 << 7))) return false; // 0b10000000

    opcode_ = (PCPOpCode)(p[1] & ((1 << 7) - 1)); // 0b01111111
    res_code_ = (PCPResCode)p[3];

    lifetime_ = ntohl(*(uint32_t*)(&p[4]));
    time_ = ntohl(*(uint32_t*)(&p[8]));

    p = &p[24];
    if (opcode_ == OP_ANNOUNCE) {
        return true;
    } else if (opcode_ == OP_MAP) {
        if (size < 24 + 36) return false;

        memcpy(&nonce_, &p[0], sizeof(PCPNonce));
        proto_ = (PCPProto)p[12];

        internal_port_ = ntohs(*(uint16_t*)(&p[16]));
        external_port_ = ntohs(*(uint16_t*)(&p[18]));

        external_address_ = Address(p[32], p[33], p[34], p[35], 0);

        return true;
    } else if (opcode_ == OP_PEER) {
        if (size < 24 + 56) return false;

        memcpy(&nonce_, &p[0], sizeof(PCPNonce));
        proto_ = (PCPProto)p[12];

        internal_port_ = ntohs(*(uint16_t*)(&p[16]));
        external_port_ = ntohs(*(uint16_t*)(&p[18]));

        external_address_ = Net::Address(p[32], p[33], p[34], p[35], 0);

        remote_port_ = ntohs(*(uint16_t*)(&p[36]));

        remote_address_ = Address(p[52], p[53], p[54], p[55], 0);

        return true;
    } else {
        return false;
    }
}

int Net::PCPResponse::Write(void *buf, int size) const {
    uint8_t *p = (uint8_t *)buf;

    if (opcode_ == OP_NONE || size < 24) return -1;

    // version
    p[0] = 2;

    // response
    p[1] = 1 << 7; // 0b10000000

    // opcode
    p[1] |= opcode_ & 0xFF;

    // reserved
    p[2] = 0;

    p[3] = res_code_ & 0xFF;

    *(uint32_t*)(&p[4]) = htonl(lifetime_);
    *(uint32_t*)(&p[8]) = htonl(time_);

    // reserved
    memset(&p[12], 0, 12);

    p = &p[24];
    if (opcode_ == OP_ANNOUNCE) {
        return 24;
    } else if (opcode_ == OP_MAP) {
        if (size < 24 + 36) return -1;

        memcpy(&p[0], &nonce_, sizeof(PCPNonce));
        p[12] = proto_;
        p[13] = p[14] = p[15] = 0;

        *(uint16_t*)(&p[16]) = htons(internal_port_);
        *(uint16_t*)(&p[18]) = htons(external_port_);

        // ipv4 mapped to ipv6
        p[20] = p[21] = p[22] = p[23] = 0;
        p[24] = p[25] = p[26] = p[27] = 0;
        p[28] = p[29] = 0;
        p[30] = p[31] = 0xFF;
        p[32] = external_address_.a();
        p[33] = external_address_.b();
        p[34] = external_address_.c();
        p[35] = external_address_.d();

        return 24 + 36;
    } else if (opcode_ == OP_PEER) {
        if (size < 24 + 56) return -1;

        memcpy(&p[0], &nonce_, sizeof(PCPNonce));
        p[12] = proto_;
        p[13] = p[14] = p[15] = 0;

        *(uint16_t*)(&p[16]) = htons(internal_port_);
        *(uint16_t*)(&p[18]) = htons(external_port_);

        // ipv4 mapped to ipv6
        p[20] = p[21] = p[22] = p[23] = 0;
        p[24] = p[25] = p[26] = p[27] = 0;
        p[28] = p[29] = 0;
        p[30] = p[31] = 0xFF;
        p[32] = external_address_.a();
        p[33] = external_address_.b();
        p[34] = external_address_.c();
        p[35] = external_address_.d();

        *(uint16_t*)(&p[36]) = htons(remote_port_);
        p[38] = p[39] = 0;

        // ipv4 mapped to ipv6
        p[40] = p[41] = p[42] = p[43] = 0;
        p[44] = p[45] = p[46] = p[47] = 0;
        p[48] = p[49] = 0;
        p[50] = p[51] = 0xFF;
        p[52] = remote_address_.a();
        p[53] = remote_address_.b();
        p[54] = remote_address_.c();
        p[55] = remote_address_.d();

        return 24 + 56;
    } else {
        return -1;
    }
}

/*************** PCPSession ***************/

void Net::PCPSession::Update(unsigned int dt_ms) {
    char buf[128];

    main_timer_ += dt_ms;

    if (state_ == REQUEST_MAPPING) {
        if (main_timer_ >= request_timer_) {
            if ((MRC && request_counter_ >= MRC) ||
                (MRD && main_timer_ >= MRD * 1000)) {
                state_ = IDLE_FAILED;
                return;
            }

            PCPRequest req;
            req.MakeMapRequest(proto_, internal_port_, external_port_, lifetime_, sock_.local_addr(), nonce_);
            req.set_external_address(external_addr_);
            int size = req.Write(buf, sizeof(buf));

            sock_.Send(pcp_server_, buf, size);

            request_counter_++;
            request_timer_ += rt_ * 1000;
            rt_ = RT(rt_);
        }

        Address sender;
        int rcv_size = sock_.Receive(sender, buf, sizeof(buf));
        if (rcv_size && sender == pcp_server_) {
            PCPResponse resp;
            if (!resp.Read(buf, rcv_size)) return;

            if (resp.opcode() == OP_MAP &&
                resp.nonce() == nonce_) {
                if (resp.res_code() == PCP_RES_SUCCESS) {
                    external_addr_ = resp.external_address();
                    mapped_time_ = main_timer_;
                    state_ = IDLE_MAPPED;
                } else {
                    state_ = IDLE_FAILED;
                    err_code_ = resp.res_code();
                }
            }
        }
    } else if (state_ == IDLE_MAPPED) {
        if (main_timer_ - mapped_time_ > (5.0f/8) * lifetime_) {
            rt_ = (1 + RAND()) * IRT;
            state_ = REQUEST_MAPPING;
        }
    } else if (state_ == IDLE_FAILED) {}
}

float Net::PCPSession::RT(float rt) {
    return (1 + RAND()) * std::min<float>(2 * rt, (float)MRT);
}