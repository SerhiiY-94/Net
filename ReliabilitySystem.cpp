#pragma warning(disable : 4244)
#include "ReliabilitySystem.h"

#include <cstdio>

net::ReliabilitySystem::ReliabilitySystem(unsigned int max_sequence) : max_sequence_(max_sequence) {
    Reset();
}

void net::ReliabilitySystem::Reset() {
    remote_sequence_ = local_sequence_ = 0;
    sent_queue_.clear();
    received_queue_.clear();
    pending_ack_queue_.clear();
    acked_queue_.clear();
    sent_packets_ = recv_packets_ = 0;
    lost_packets_ = acked_packets_ = 0;
    sent_bandwidth_ = 0.0f;
    acked_bandwidth_ = 1.0f;
    rtt_ = 0.0f;
    rtt_maximum_ = 1.0f;
}

void net::ReliabilitySystem::PacketSent(void *data, int size) {
    if(sent_queue_.exists(local_sequence_)) {
        printf("local sequence %d exists\n", local_sequence_);
        for(PacketQueue::iterator it = sent_queue_.begin(); it != sent_queue_.end(); it++) {
            printf(" + %d\n", it->sequence);
        }
    }
    assert(!sent_queue_.exists(local_sequence_));
    assert(!pending_ack_queue_.exists(local_sequence_));

	PacketData pdata(local_sequence_, 0, size);

    sent_queue_.push_back(pdata);
    pending_ack_queue_.push_back(pdata);
    sent_packets_++;
    local_sequence_++;
    if(local_sequence_ > max_sequence_) {
        local_sequence_ = 0;
    }
}

void net::ReliabilitySystem::PacketReceived(unsigned int sequence, int size) {
    recv_packets_++;
    if(received_queue_.exists(sequence)) {
        return;
    }

	PacketData data(sequence, 0, size);

    received_queue_.push_back(data);
    if(sequence_more_recent(sequence, remote_sequence_, max_sequence_)) {
        remote_sequence_ = sequence;
    }
}

void net::ReliabilitySystem::Update(float dt_s) {
    acks_.clear();
    AdvanceQueueTime(dt_s);
    UpdateQueues();
    UpdateStats();
}

void net::ReliabilitySystem::AdvanceQueueTime(float dt_s) {
    for(auto &p : sent_queue_) {
        p.time += dt_s;
    }
    for(auto &p : received_queue_) {
        p.time += dt_s;
    }
    for(auto &p : pending_ack_queue_) {
        p.time += dt_s;
    }
    for(auto &p : acked_queue_) {
        p.time += dt_s;
    }
}

void net::ReliabilitySystem::UpdateQueues() {
    const float epsilon = 0.001f;

    while(sent_queue_.size() && sent_queue_.front().time > rtt_maximum_ + epsilon) {
        sent_queue_.pop_front();
    }

    if (received_queue_.size()) {
        const unsigned int latest_sequence = received_queue_.back().sequence;
        const unsigned int minimum_sequence =
            latest_sequence >= 34 ? (latest_sequence - 34) : max_sequence_ - (34 - latest_sequence);
        while (received_queue_.size() &&
              !sequence_more_recent(received_queue_.front().sequence, minimum_sequence, max_sequence_)) {
            received_queue_.pop_front();
        }
    }

    while (acked_queue_.size() && acked_queue_.front().time > rtt_maximum_ * 2 - epsilon) {
        acked_queue_.pop_front();
    }

    while (pending_ack_queue_.size() && pending_ack_queue_.front().time > rtt_maximum_ + epsilon) {
        pending_ack_queue_.pop_front();
        lost_packets_++;
    }
}

void net::ReliabilitySystem::UpdateStats() {
    int sent_bytes_per_second = 0;

	for (PacketQueue::iterator it = sent_queue_.begin(); it != sent_queue_.end(); ++it) {
		sent_bytes_per_second += it->size;
	}

    int acked_packets_per_second	= 0;
    int acked_bytes_per_second		= 0;

    for (PacketQueue::iterator it = acked_queue_.begin(); it != acked_queue_.end(); ++it) {
        if (it->time >= rtt_maximum_) {
            acked_packets_per_second++;
            acked_bytes_per_second += it->size;
        }
    }

    sent_bytes_per_second /= rtt_maximum_;
    acked_bytes_per_second /= rtt_maximum_;
    sent_bandwidth_ = sent_bytes_per_second * (8 / 1000.0f);
    acked_bandwidth_ = acked_bytes_per_second * (8 / 1000.0f);
}

void net::ReliabilitySystem::GetAcks(unsigned int **acks, int &count) {
	if (!acks_.empty()) {
		*acks = &acks_[0];
	}
    count = (int)acks_.size();
}

int net::ReliabilitySystem::bit_index_for_sequence(unsigned int sequence, unsigned int ack, unsigned int max_sequence) {
    assert(sequence != ack);
    assert(!sequence_more_recent(sequence, ack, max_sequence));
    if(sequence > ack) {
        assert(ack < 33);
        assert(sequence <= max_sequence);
        return ack + (max_sequence - sequence);
    } else {
        assert(ack >= 1);
        assert(sequence <= ack - 1);
        return ack - 1 - sequence;
    }
}

unsigned int net::ReliabilitySystem::generate_ack_bits(unsigned int ack,
                                                  const PacketQueue &received_queue,
                                                  unsigned int max_sequence) {
    unsigned int ack_bits = 0;
    for(PacketQueue::const_iterator itor = received_queue.begin(); itor != received_queue.end(); itor++) {
        if(itor->sequence == ack || sequence_more_recent(itor->sequence, ack, max_sequence)) {
            break;
        }
        int bit_index = bit_index_for_sequence(itor->sequence, ack, max_sequence);
        if(bit_index <= 31) {
            ack_bits |= 1 << bit_index;
        }
    }
    return ack_bits;
}

void net::ReliabilitySystem::process_ack(unsigned int ack,
										 unsigned int ack_bits,
										 PacketQueue &pending_ack_queue,
										 PacketQueue &acked_queue,
										 std::vector<unsigned int> &acks,
										 unsigned int &acked_packets,
										 float &rtt,
										 unsigned int max_sequence) {
    if (pending_ack_queue.empty()) {
        return;
    }
    PacketQueue::iterator it = pending_ack_queue.begin();
    while (it != pending_ack_queue.end()) {
        bool acked = false;

        if (it->sequence == ack) {
            acked = true;
        } else if (!sequence_more_recent(it->sequence, ack, max_sequence)) {
            int bit_index = bit_index_for_sequence(it->sequence, ack, max_sequence);
            if (bit_index <= 31) {
                acked = (ack_bits >> bit_index) & 1;
            }
        }

        if (acked) {
            rtt += (it->time - rtt) * 0.1f;

            acked_queue.insert_sorted(*it, max_sequence);
            acks.push_back(it->sequence);
            acked_packets++;
            it = pending_ack_queue.erase(it);
        } else {
            ++it;
        }
    }
}