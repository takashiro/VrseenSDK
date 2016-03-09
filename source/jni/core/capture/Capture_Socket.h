#pragma once

#include "vglobal.h"

#include "Capture.h"
#include "Capture_Packets.h"
#include "Capture_Thread.h"

#if defined(OVR_CAPTURE_POSIX)
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#pragma once

NV_NAMESPACE_BEGIN
namespace Capture
{

    class SocketAddress
    {
        friend class Socket;
        public:
            static SocketAddress Any(UInt16 port);
            static SocketAddress Broadcast(UInt16 port);

            SocketAddress(void);

        private:
        #if defined(OVR_CAPTURE_POSIX)
            struct sockaddr_in m_addr;
        #endif
    };

    class Socket
    {
        public:
            enum Type
            {
                Type_Stream = 0,
                Type_Datagram,
            };
        public:
            static Socket *Create(Type type);

            void Release(void);

            bool    SetBroadcast(void);
            bool    Bind(const SocketAddress &addr);
            bool    Listen(UInt32 maxConnections);
            Socket *Accept(SocketAddress &addr);
            void    Shutdown(void);
            bool    Send(const void *buffer, UInt32 size);
            bool    SendTo(const void *buffer, UInt32 size, const SocketAddress &addr);
            UInt32  Receive(void *buffer, UInt32 size);

        private:
            Socket(void);
            ~Socket(void);

        private:
        #if defined(OVR_CAPTURE_POSIX)
            static const int s_invalidSocket = -1;
            int              m_socket;
            // these pipes are used to wake up sleeping select() calls...
            int              m_sendPipe;
            int              m_recvPipe;
        #endif
            bool             m_okay;
    };

    class ZeroConfigHost : public Thread
    {
        public:
            static ZeroConfigHost *Create(UInt16 udpPort, UInt16 tcpPort, const char *packageName);

            void Release(void);

        private:
            ZeroConfigHost(Socket *broadcastSocket, UInt16 udpPort, UInt16 tcpPort, const char *packageName);
            ~ZeroConfigHost(void);

        private:
            virtual void OnThreadExecute(void);

        private:
            Socket          *m_socket;
            UInt16           m_udpPort;
            ZeroConfigPacket m_packet;
    };

} // namespace Capture
NV_NAMESPACE_END
