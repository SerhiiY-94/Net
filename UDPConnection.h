#ifndef UDP_CONNECTION_H
#define UDP_CONNECTION_H

#include "Socket.h"

namespace net {
	const int MAX_PACKET_SIZE = 4096;
	class UDPConnection {
	public:
		enum Mode { NONE, CLIENT, SERVER };

		UDPConnection(unsigned int protocol_id, float timeout_s);
		~UDPConnection();

		const UDPSocket &socket() const { return socket_; }

		void Start(int port);
		void Stop();
		void Listen();
		void Connect(const Address &address);
		virtual void Update(float dt_s);

		virtual bool SendPacket(const unsigned char data[], int size);
		virtual int ReceivePacket(unsigned char data[], int size);

		bool listening() const {
			return state_ == LISTENING;
		}
		bool connecting() const {
			return state_ == CONNECTING;
		}
		bool connect_failed() const {
			return state_ == CONNECTFAIL;
		}
		bool connected() const {
			return state_ == CONNECTED;
		}

		Mode mode() const {
			return mode_;
		}

        bool running() const {
            return running_;
        }

		Address address() const {
			return address_;
		}

		Address local_addr() const {
			return socket_.local_addr();
		}

		float timeout_acc() const {
			return timeout_acc_;
		}

	protected:
		virtual void OnStart() {}
		virtual void OnStop() {}
		virtual void OnConnect() {}
		virtual void OnDisconnect() {}
	private:
		void ClearData() {
			state_ = DISCONNECTED;
			timeout_acc_ = 0;
			address_ = Address();
		}
		enum State { DISCONNECTED, LISTENING, CONNECTING, CONNECTFAIL, CONNECTED };

		unsigned int protocol_id_;
		float timeout_s_, timeout_acc_;

		bool running_;
		Mode mode_;
		State state_;
		UDPSocket socket_;
		Address address_;
	};
}

#endif // UDP_CONNECTION_H
