#include "VarContainer.h"

#include <stdexcept>

template<>
void net::VarContainer::SaveVar<net::VarContainer>(const Var<VarContainer> &v) {
    assert(CheckHashes(v.hash_.hash));
	Packet pack = v.Pack();

	header_.push_back(v.hash_.hash);
	header_.push_back(data_bytes_.size());

	data_bytes_.insert(data_bytes_.end(), pack.begin(), pack.end());
}

template<>
bool net::VarContainer::LoadVar<net::VarContainer>(Var<VarContainer> &v) const {
    for (unsigned int i = 0; i < header_.size(); i += 2) {
        if (header_[i] == (int_type)v.hash_.hash) {
            size_t len = i < header_.size() - 2 ? (header_[(i + 2) + 1] - header_[i + 1]) : (data_bytes_.size() - header_[i + 1]);
			v.UnPack(&data_bytes_[header_[i + 1]], len);
            return true;
        }
    }
    return false;
}

net::Packet net::VarContainer::Pack() const {
	Packet dst;

	int_type num_vars     = (int_type)header_.size() / 2;
	int_type num_vars_beg = (int_type)dst.size();
	int_type data_size    = (int_type)data_bytes_.size();
	dst.resize(num_vars_beg + sizeof(int_type) * 2);
	memcpy(&dst[num_vars_beg], &num_vars, sizeof(int_type));
	memcpy(&dst[num_vars_beg + sizeof(int_type)], &data_size, sizeof(int_type));

	int_type header_beg  = (int_type)dst.size();
	int_type header_size = (int_type)(header_.size() * sizeof(int_type));
    dst.resize(header_beg + header_size);
    memcpy(&dst[header_beg], &header_[0], header_size);

	int_type data_beg = (int_type)dst.size();
    dst.resize(data_beg + data_bytes_.size());
    memcpy(&dst[data_beg], &data_bytes_[0], data_bytes_.size());

    return dst;
}
bool net::VarContainer::UnPack(const Packet &pack) {
	return UnPack(&pack[0], pack.size());
}

bool net::VarContainer::UnPack(const unsigned char *pack, size_t len) {
    if (len < 8) return false;

	int_type num_vars, data_size;
	int_type num_vars_beg = 0;
	memcpy(&num_vars, &pack[0], sizeof(int_type));
	memcpy(&data_size, &pack[sizeof(int_type)], sizeof(int_type));

	int_type header_beg = (int_type)(num_vars_beg + sizeof(int_type) * 2);
	int_type header_size = (int_type)(2 * num_vars * sizeof(int_type));
	if (header_size + 8 > len || header_beg > header_size) {
        return false;
    }
	header_.resize(2 * num_vars);
	memcpy(&header_[0], &pack[header_beg], header_size);

	int_type data_beg = (int_type)(header_beg + header_size);
	data_bytes_.resize(data_beg + data_size);
	memcpy(&data_bytes_[0], &pack[data_beg], data_size);

	return true;
}

size_t net::VarContainer::size() const {
	return header_.size() / 2;
}

void net::VarContainer::clear() {
	header_.clear();
	data_bytes_.clear();
}