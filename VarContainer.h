#ifndef VAR_CONTAINER_H
#define VAR_CONTAINER_H

#include <cassert>
#include <vector>

#include "Types.h"
#include "Var.h"

namespace net {
typedef std::vector<unsigned char> Packet;
class VarContainer {
    std::vector<le_uint32> header_;
    std::vector<unsigned char> data_bytes_;

	bool CheckHashes(le_uint32 hash) const {
		for (unsigned int i = 0; i < header_.size(); i += 2) {
			if (header_[i] == hash) {
				return false;
			}
		}
		return true;
	}
public:
    template<class T>
    void SaveVar(const Var<T> &v) {
		assert(CheckHashes(v.hash_.hash));
        le_uint32 var_beg = (le_uint32)data_bytes_.size();
        header_.push_back(v.hash_.hash);
        header_.push_back(var_beg);

        le_uint32 data_beg = (le_uint32)data_bytes_.size();
        data_bytes_.resize(var_beg + sizeof(T));

        memcpy(&data_bytes_[data_beg], v.p_val(), sizeof(T));
    }
    template<class T>
    bool LoadVar(Var<T> &v) const {
        for (unsigned int i = 0; i < header_.size(); i += 2) {
            if (header_[i] == v.hash_.hash) {
                memcpy(v.p_val(), &data_bytes_[header_[i + 1]], sizeof(T));
                return true;
            }
        }
		return false;
    }
	template<class T>
	void UpdateVar(const Var<T> &v) {
		static_assert(!std::is_same<T, VarContainer>::value, "Cannot update VarContainer");
		for (unsigned int i = 0; i < header_.size(); i += 2) {
			if (header_[i] == v.hash_.hash) {
				memcpy(&data_bytes_[header_[i + 1]], v.p_val(), sizeof(T));
				return;
			}
		}
		SaveVar(v);
	}
	Packet Pack() const;
    bool UnPack(const Packet &pack);
	bool UnPack(const unsigned char *pack, size_t len);
	size_t size() const;
	void clear();
};
template<>
void VarContainer::SaveVar<VarContainer>(const Var<VarContainer> &v);
template<>
bool VarContainer::LoadVar<VarContainer>(Var<VarContainer> &v) const;

}

#endif // VAR_CONTAINER_H