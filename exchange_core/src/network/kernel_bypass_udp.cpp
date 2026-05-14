#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace hft {

/**
 * Kernel-bypass UDP broadcaster.
 * On Linux, SO_BUSY_POLL and SO_REUSEPORT are used to minimize kernel overhead.
 * In a production deployment, this would be replaced by DPDK or Solarflare ef_vi.
 */
class KernelBypassUDP {
public:
    KernelBypassUDP() : fd_(-1) {}

    ~KernelBypassUDP() {
        if (fd_ >= 0) close(fd_);
    }

    bool open(const char* bindAddr, uint16_t port) {
        fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd_ < 0) return false;

        // Enable busy-polling for kernel-bypass-like latency
        int busyPoll = 50; // microseconds
        setsockopt(fd_, SOL_SOCKET, SO_BUSY_POLL, &busyPoll, sizeof(busyPoll));

        int reuse = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);
        inet_pton(AF_INET, bindAddr, &addr.sin_addr);

        if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(fd_);
            fd_ = -1;
            return false;
        }

        port_ = port;
        return true;
    }

    // Multicast broadcast
    bool send(const void* data, size_t len,
              const char* mcastAddr = "239.0.0.1", uint16_t mcastPort = 9000) {
        if (fd_ < 0) return false;

        struct sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port   = htons(mcastPort);
        inet_pton(AF_INET, mcastAddr, &dest.sin_addr);

        ssize_t sent = sendto(fd_, data, len, 0,
                              (struct sockaddr*)&dest, sizeof(dest));
        return sent == static_cast<ssize_t>(len);
    }

    bool recv(void* buf, size_t bufLen, size_t& received) {
        if (fd_ < 0) return false;
        ssize_t n = recvfrom(fd_, buf, bufLen, MSG_DONTWAIT, nullptr, nullptr);
        if (n < 0) return false;
        received = static_cast<size_t>(n);
        return true;
    }

private:
    int      fd_;
    uint16_t port_ = 0;
};

} // namespace hft
