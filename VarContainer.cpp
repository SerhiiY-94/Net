#include "VarContainer.h"

#include <stdexcept>

using namespace net;

template<>
void VarContainer::SaveVar<VarContainer>(const Var<VarContainer> &v) {
    assert(CheckHashes(v.hash_.hash));
	Packet pack = v.Pack();

	header_.push_back(v.hash_.hash);
	header_.push_back(data_bytes_.size());

	data_bytes_.insert(data_bytes_.end(), pack.begin(), pack.end());
}

template<>
bool VarContainer::LoadVar<VarContainer>(Var<VarContainer> &v) const {
    for (unsigned int i = 0; i < header_.size(); i += 2) {
        if (header_[i] == v.hash_.hash) {
            size_t len = i < header_.size() - 2 ? (header_[(i + 2) + 1] - header_[i + 1]) : (data_bytes_.size() - header_[i + 1]);
			v.UnPack(&data_bytes_[header_[i + 1]], len);
            return true;
        }
    }
    return false;
}

Packet VarContainer::Pack() const {
	Packet dst;

	le_uint32 num_vars = (le_uint32)header_.size() / 2;
	le_uint32 num_vars_beg = (le_uint32)dst.size();
	le_uint32 data_size = (le_uint32)data_bytes_.size();
    dst.resize(num_vars_beg + sizeof(le_uint32) * 2);
    memcpy(&dst[num_vars_beg], &num_vars, sizeof(le_uint32));
	memcpy(&dst[num_vars_beg + sizeof(le_uint32)], &data_size, sizeof(le_uint32));

	le_uint32 header_beg = (le_uint32)dst.size();
	le_uint32 header_size = (le_uint32) (header_.size() * sizeof(le_uint32));
    dst.resize(header_beg + header_size);
    memcpy(&dst[header_beg], &header_[0], header_size);

	le_uint32 data_beg = (le_uint32)dst.size();
    dst.resize(data_beg + data_bytes_.size());
    memcpy(&dst[data_beg], &data_bytes_[0], data_bytes_.size());

    return dst;
}
bool VarContainer::UnPack(const Packet &pack) {
	return UnPack(&pack[0], pack.size());
}

bool VarContainer::UnPack(const unsigned char *pack, size_t len) {
    if (len < 8) return false;

	le_uint32 num_vars, data_size;
	le_uint32 num_vars_beg = 0;
	memcpy(&num_vars, &pack[0], sizeof(le_uint32));
	memcpy(&data_size, &pack[sizeof(le_uint32)], sizeof(le_uint32));

	le_uint32 header_beg = (le_uint32)(num_vars_beg + sizeof(le_uint32) * 2);
	le_uint32 header_size = (le_uint32)(2 * num_vars * sizeof(le_uint32));
	if (header_size + 8 > len || header_beg > header_size) {
        return false;
    }
	header_.resize(2 * num_vars);
	memcpy(&header_[0], &pack[header_beg], header_size);

	le_uint32 data_beg = (le_uint32)(header_beg + header_size);
	data_bytes_.resize(data_beg + data_size);
	memcpy(&data_bytes_[0], &pack[data_beg], data_size);

	return true;
}

size_t VarContainer::size() const {
	return header_.size() / 2;
}

void VarContainer::clear() {
	header_.clear();
	data_bytes_.clear();
}