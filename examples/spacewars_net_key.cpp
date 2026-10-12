/*
 * Author: Svenson Han Göthel und Sven Göthel
 * Copyright (c) 2023 Gothel Software e.K.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <asm-generic/socket.h>
#include <pthread.h>
#include <sys/socket.h>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <numbers>
#include <pixel/pixel3f.hpp>
#include <pixel/pixel4f.hpp>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include "pixel/pixel.hpp"
#include <jau/utils.hpp>
#include <jau/cpp_pragma.hpp>

#include <algorithm>
#include <cinttypes>
#include <string>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include <cctype>
#include <thread>

#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

#include <netdb.h>
#include <cstdlib>
#include <fstream>
#include <fcntl.h>

#include <unistd.h>   // for fork(), execvp()
#include <sys/wait.h> // for waitpid()

using namespace jau;
using namespace jau::si_f32_literals;

static pixel::input_event_t event;
constexpr static int player_id_1 = 1;
constexpr static int player_id_2 = 2;
constexpr static int player_id_3 = 3;
constexpr static float spaceship_height = 10.0f; // [m]
constexpr static float space_height = spaceship_height*30.0f; // [m]
constexpr static float sun_gravity = 28 * 8.0f; // [m/s^2]
static const uint8_t rgba_white[/*4*/] = { 255, 255, 255, 255 };
static const uint8_t rgba_yellow[/*4*/] = { 255, 255, 0, 255 };
static const uint8_t rgba_red[/*4*/] = { 255, 0, 0, 255 };
static const uint8_t rgba_green[/*4*/] = { 0, 255, 0, 255 };
// static const uint8_t rgba_blue[/*4*/] = { 0, 0, 255, 255 };
static const float text_lum = 0.75f;
static const pixel::f4::vec_t vec4_text_color(text_lum, text_lum, text_lum, 1.0f);
static bool debug_gfx = false;
static bool show_ship_velo = false;
static bool cloak_enabled = true;
static bool global_is_server = true;
static int global_port = 8081;
static std::string global_server_name;
static bool draw_grip = false;
static size_t sendedbyte = 0;

class FloatBin;

class FloatBin {
  private:
    std::vector<float> m_data;
    size_t m_ridx;
    
  public:
    FloatBin(const size_t& reserve = 512)
    : m_ridx(0)
    { m_data.reserve(reserve); }

    FloatBin(const FloatBin&)                              = default;
    FloatBin(FloatBin&&)                                   = default;
    FloatBin& operator=(const FloatBin&)                   = default;
    FloatBin& operator=(FloatBin&&)                        = default;
    bool      operator==(const FloatBin& o) const noexcept = default;

    void clear() noexcept {
        m_data.clear();
        m_ridx = 0;
    }
    void rewind() noexcept {
        m_ridx = 0;
    }
    constexpr size_t size() const noexcept { return m_data.size(); }
    constexpr size_t readPos() const noexcept { return m_ridx; }
    constexpr size_t remaining() const noexcept { const size_t s = m_data.size(); return m_ridx < s ? s - m_ridx : 0; }
    constexpr const std::vector<float>& data() const noexcept { return m_data; }
    constexpr std::vector<float>& data() noexcept { return m_data; }
    
    constexpr float get() noexcept {
        return m_ridx < size() ? m_data[m_ridx++] : 0;
    }
    
    constexpr void put(float v) noexcept { m_data.push_back(v); }    
};
 
std::vector<std::string> encode_keys = {"Up", "Down", "Left", "Right", 
                                        "Action1", "Action2", "Action3", "Action4"};
namespace io {
    namespace impl {
        void printError(std::ostream* out, const char *format, va_list args) { 
            const int e = errno;
            char buf[1024];
            std::string_view vbuf;
            {
                // -Wformat=2 -> -Wformat -Wformat-nonliteral -Wformat-security -Wformat-y2k
                // -Wformat=2 -Wformat-overflow=2 -Wformat-signedness
                PRAGMA_DISABLE_WARNING_PUSH
                PRAGMA_DISABLE_WARNING_FORMAT_NONLITERAL
                PRAGMA_DISABLE_WARNING_FORMAT_SECURITY
                vbuf = std::string_view(buf, std::vsnprintf(buf, sizeof(buf), format, args));
                PRAGMA_DISABLE_WARNING_POP
            }
            if (out) {
                *out << "Error: " << vbuf << ": " << e << ", " << ::strerror(e) << "\n";
            }
            if (out != &std::cerr) {
                std::cerr << "Error: " << vbuf << ": " << e << ", " << ::strerror(e) << "\n";
            }
        }
    }

    __attribute__((format (printf,2,3)))
    void printError(std::ostream* out, const char *format, ...) { 
        va_list args;
        va_start (args, format);
        impl::printError(out, format, args);
        va_end (args);
    }

    __attribute__((format (printf,1,2)))
    void printError(const char *format, ...) { 
        va_list args;
        va_start (args, format);
        impl::printError(nullptr, format, args);
        va_end (args);
    }
     
    void printDebug(const std::string& msg) { 
        std::cerr << "Debug: " << msg << "\n"; 
    }
    
    namespace net {
            bool set_blocking(int fd, bool blocking) {
                const int oflags = fcntl(fd, F_GETFL, 0);
                if( oflags < 0 ) { 
                    io::printError("Get fd flags failed");
                    return false;
                }
                int nflags;
                if(blocking) {
                    nflags = oflags & ~O_NONBLOCK;
                } else {
                    nflags = oflags | O_NONBLOCK;
                }
                if (fcntl(fd, F_SETFL, nflags) ) {
                    io::printError(blocking ? "Set fd blocking failed" : "Set fd non-blocking failed");
                    return false;
                }
                return true;
            }
            bool set_reuse(int fd) {
                const int enable = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
                    io::printError("SockOpt SO_REUSEADDR");
                    return false;        
                }    
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
                    io::printError("SockOpt SO_REUSEPORT");        
                    return false;        
                }                
                return true;        
            }
            bool sendRaw(int sock, const void* data, size_t len) noexcept {
                sendedbyte += len;
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
                while( len > 0 ) {
                    const ssize_t r = ::send(sock, bytes, len, 0);
                    if( r < 0 ) {
                        // FIXME: Check errno
                        printError("send");
                        return false;
                    } else {
                        len   -= r;
                        bytes += r;
                    }
                }
                return true;
            }
            bool sendText(int sock, const std::string& message) {
                if( sock < 0 ) {
                    return false;
                }
                const uint64_t len = message.length();  // FIXME: equal host byteorder?
                return sendRaw(sock, &len, sizeof(len)) &&
                       sendRaw(sock, message.c_str(), len);
            }

            bool sendText(int sock, const FloatBin& message) {
                if( sock < 0 ) {
                    return false;
                }
                const uint64_t len = sizeof(message);
                printf("Send length : %zu\n", len);
                printf("Send pointer: %p\n", (void*)&message.data());
                
                return sendRaw(sock, &len, sizeof(len)) &&
                       sendRaw(sock, (void*)&message.data(), len);
            }

            /**
                [len][data][crc]
            */
            bool receiveRaw(std::ofstream* log, int sock, void* data, size_t len, bool blocking) noexcept {
                uint8_t* bytes = reinterpret_cast<uint8_t*>(data);
                size_t   got   = 0;
                while( got < len ) {
                    ssize_t r = ::recv(sock, bytes, len - got, 0);
                    if( r < 0 ) {
                        // FIXME: Check errno
                        if (errno == EAGAIN) {
                            if (blocking) {
                                continue;
                            } else if( 0 == got) {
                                if(!global_is_server) {
                                    // Breakpoint
                                    printError(log, "receiveRaw.0a");
                                }
                                //printError(log, "receiveRaw.0b");
                                //std::string s = global_is_server ? "Server" : "Client";
                                //std::cerr << s;
                                return true;
                            }                     
                        }   
                        printError(log, "receiveRaw.1: data %p, len %zu, blocking %d", data, len, blocking);
                        return false;
                    } else if( 0 == got && r == 0 ) {
                        return true;
                    } else {
                        got   += r;
                        bytes += r;
                    }
                }
                return true;
            }
            bool receiveText(std::ofstream* log, int sock, std::string& message) {
                if( sock < 0 ) {
                    printError("!sock");
                    return false;
                }
                uint64_t len = 0;  // FIXME: equal host byteorder?
                printError("sizeof(len): %zu", sizeof(len));
                if( !receiveRaw(log, sock, &len, sizeof(len), false) ) {
                    printError(log, "receiveText: error");
                    return false;
                }
                printError("len: %zu", len);
                if (log) {
                    *log << "[receiveText]: len = " << len << "\n";
                    log->flush();
                }
                if( 0 == len ) {
                    // printError("receiveText: zero of " + std::to_string(len) + ", msg " + message);
                    message.clear();
                    return true;
                }
                message.reserve(len + 1);  // incl. EOS
                message.resize(len);       // excl. EOS
                // printError("receiveText: OK msg " + message);
                bool r = receiveRaw(log, sock, &message[0], len, true);
                if (log) {
                    if (r) {
                        *log << "[receiveText(" << getElapsedMillisecond() << ")]: msg = " << message << "\n";
                    } else {
                        *log << "[receiveText]: error\n";
                    }
                    log->flush();
                }
                return r;
            }

            bool receiveText(std::ofstream* log, int sock, FloatBin& message, float& typ) {
                if( sock < 0 ) {
                    printError("!sock");
                    return false;
                }
                uint64_t len = 0;  // FIXME: equal host byteorder?
                if( !receiveRaw(log, sock, &len, sizeof(len), false) ) {
                    printError(log, "receiveText: error");
                    return false;
                }
                if (log) {
                    *log << "[receiveText]: len = " << len << "\n";
                    log->flush();
                }
                if( 0 == len ) {
                    // printError("receiveText: zero of " + std::to_string(len) + ", msg " + message);
                    message.clear();
                    return true;
                }
                bool r = receiveRaw(log, sock, &message.data()[0], len, true);
                typ = message.size() == 1 ? 0 : 1;
                if (log) {
                    if (r) {
                        *log << "[receiveText(" << getElapsedMillisecond() << ")]: msg = ";
                        for(const float& f : message.data()) {
                            std::cout << f << ", ";
                        }
                        std::cout << std::endl;
                    } else {
                        *log << "[receiveText]: error\n";
                    }
                    log->flush();
                }
                return r;
            }

            template <class DecodedType, class Decode>
            bool receiveDecoded(std::ofstream* log, int sock, DecodedType& result, Decode decode) {
                std::string str;
                do {
                    if( !receiveText(log, sock, str) ) {
                        return false;
                    }
                } while( !decode(str, result) );
                return true;
            }
        }

    class IOEnv {
      private:
        std::unique_ptr<std::ifstream> m_user_filein;
        std::unique_ptr<std::ifstream> m_net_filein;
        std::ofstream m_user_rec;
        std::ofstream m_net_rec;
      public:
        IOEnv(const std::string& userRecFilename, const std::string& netRecFilename) 
        : m_user_filein(nullptr), m_net_filein(nullptr)
        {
            m_user_rec = std::ofstream(userRecFilename, std::ios::out | std::ios::trunc);
            m_net_rec = std::ofstream(netRecFilename, std::ios::out | std::ios::trunc);
            std::cout << "userRec " << userRecFilename << "\n";
            std::cout << "netRec " << netRecFilename << "\n";
        }
        ~IOEnv() = default;
        void setUserIn(const std::string& userInFilename) {
            m_user_filein = std::make_unique<std::ifstream>(userInFilename, std::ios::in);
            std::cout << "setUserIn " << userInFilename << "\n";
        }
        void unsetUserIn() {
            m_user_filein = nullptr;
        }
        void setNetIn(const std::string& netInFilename) {
            m_net_filein = std::make_unique<std::ifstream>(netInFilename, std::ios::in);
            std::cout << "setUserIn " << netInFilename << "\n";
        }
        void unsetNetIn() {
            m_net_filein = nullptr;
        }
        std::istream& userIn() noexcept { 
            return m_user_filein && m_user_filein->good() ? *m_user_filein : std::cin; 
        }
        std::unique_ptr<std::ifstream>& netIn() noexcept { 
            return m_net_filein; 
        }
        bool netInValid() noexcept { 
            return m_net_filein && m_net_filein->good(); 
        }
        bool getFileLine(std::unique_ptr<std::ifstream>& in, std::string& s) {
            if(in && in->good()) {
                std::getline(*in, s);
                return in->good();
            }
            return false; 
        }
        
        void getUserLine(std::string& s) {
            if(getFileLine(m_user_filein, s)){
                std::cout << s << "\n";
            } else {
                std::getline(std::cin, s);
            }
            userRec() << s << "\n";
            userRec().flush();
        }
        
        bool getNetLine(const float time, int sock, std::string& s) {
            bool r = false;//getFileLine(m_net_filein, s);
            if(!r){
                r = net::receiveText(&netRec(), sock, s);
            }
            if(r){
                netRec() << "[" << time << "]: " << s << "\n";
                netRec().flush();
            }
            return r;
        }
        
        bool getNetLine(const float time, int sock, FloatBin& s, float& typ) {
            (void)time;
            bool r = false;//getFileLine(m_net_filein, s);
            if(r){
                netRec() << "[" << time << "]: ";
                for(const float& f : s.data()) {
                    netRec() << f << ", ";
                }
                netRec() << "\n";
                netRec().flush();
            }
            if(!r){
                r = net::receiveText(&netRec(), sock, s, typ);
            }
            return r;
        }
        
        bool sendNetLine(int sock, const std::string& msg){
            if(m_net_filein){
                return true;
            }
            userRec() << msg << "\n";
            return net::sendText(sock, msg);
        }
        
        bool sendNetLine(int sock, const FloatBin& msg){
            if(m_net_filein){
                return true;
            }
            for(const float& f : msg.data()) {
                userRec() << f << ", ";
            }
            userRec() << "\n";
            return net::sendText(sock, msg);
        }
        
        std::ofstream& userRec() noexcept { return m_user_rec; }
        std::ofstream& netRec() noexcept { return m_net_rec; }
    };
    std::unique_ptr<IOEnv> ioEnv;
    
}

///
///
///

int serverWaitForClient(int port)
{
    // creating socket
    int serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (!io::net::set_reuse(serverSocket)) {
        io::printError("Server reuse addr/port");
        ::close(serverSocket);
        return -1;        
    }
    // binding socket.
    if( 0 != ::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) ) {
        io::printError("Server Bind");
        ::close(serverSocket);
        return -1;
    }

    // listening to the assigned socket
    if( 0 != ::listen(serverSocket, 1) ) {
        io::printError("Server Listen(1)");
        ::close(serverSocket);
        return -1;
    }

    if(io::ioEnv->netInValid()){
        ::close(serverSocket);
        return -1;
    }
    
    // accepting connection request
    std::cerr << "Server: Accept Connections...\n";
    int conn = ::accept(serverSocket, nullptr, nullptr);
    std::cerr << "Server["<< conn <<"]: Connected " << conn << "\n";
    ::close(serverSocket);
    if(conn < 0) {
        io::printError("Connecting of server failed");
    } else {
        std::cerr << "Connecting of server successful\n";
        if (!io::net::set_blocking(conn, false)) {
            ::close(conn);
            return -1;            
        }
    }
    return conn;
}

class SocketAddress {
  private:
    struct sockaddr m_addr;
    size_t m_size;
    
  public:
    SocketAddress() noexcept
    : m_addr(), m_size(0) {
        ::memset(&m_addr,  0, sizeof(struct sockaddr));
    }
    SocketAddress(const struct sockaddr* addr, size_t size) noexcept 
    : m_addr(), m_size(size) {
        ::memcpy(&m_addr, addr, size);
    }
    SocketAddress(const std::string& server_name, int port) 
    : m_addr(), m_size(0) {
      struct addrinfo hints;
      struct addrinfo *res;
    
      ::memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET;
    
      std::string port_s = std::to_string(port);
      int ret = ::getaddrinfo(server_name.c_str(), port_s.c_str(), &hints, &res);
      if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
      } else {
        m_size = res->ai_addrlen;
        ::memcpy(&m_addr, res->ai_addr, m_size);
        ::freeaddrinfo(res);
      }
    }
    
    constexpr const struct sockaddr& addr() const noexcept { return m_addr; }
    constexpr struct sockaddr& addr() noexcept { return m_addr; }
    constexpr size_t size() const noexcept { return m_size; }
    
    static std::string to_string(const struct sockaddr* a, size_t len) {
        std::string s("{");
        s.append(std::to_string(len)).append(": ");
        if( AF_INET == a->sa_family ) {
            s.append("ipv4:");
        } else if( AF_INET6 == a->sa_family ) {
            s.append("ipv6:");
        } else {
            s.append("u").append(std::to_string(a->sa_family)).append(":");
        }
        char host[256], srv[100];
        if( 0 == ::getnameinfo(a, len, host, sizeof(host), srv, sizeof(srv),
                               NI_NUMERICHOST|NI_NUMERICSERV) ) {
            s.append(host).append(":").append(srv);
        } else {
            s.append("undef");
        }
        s.append("}");
        return s;        
    }
    std::string toString() const {
        return to_string(&m_addr, m_size);
    }
};

std::ostream& operator<<(std::ostream& out, const SocketAddress& v) {
    return out << v.toString();
}

int clientConnectToServer(const std::string& server_name, int port)
{
    
    if(io::ioEnv->netInValid()){
        return -1;
    }    
    SocketAddress sa(server_name, port);
    if( !sa.size() ) {
        return -1;
    }
    // creating socket
    int conn = ::socket(AF_INET, SOCK_STREAM, 0);
    if (!io::net::set_reuse(conn)) {
        io::printError("Client reuse addr/port");
        ::close(conn);
        return -1;        
    }
    if (!io::net::set_blocking(conn, true)) {
        ::close(conn);
        return -1;            
    }        
    // sending connection request
    std::cerr << "Client: Connecting to " << sa << "\n";

    int res;
    if( 0 != (res = ::connect(conn, &sa.addr(), sa.size())) ) {
        io::printError("Connecting of client failed: %d", res);
        ::close(conn);
        return -1;
    }
    if(conn < 0) {
        io::printError("Connecting of client failed");
    } else {
        std::cerr << "Connecting of client successful";
        if (!io::net::set_blocking(conn, false)) {
            ::close(conn);
            return -1;            
        }        
    }
    return conn;
}

static constexpr bool matchAngle(float r1, float r2) noexcept {
    return std::abs(r1-r2) < 0.000001f;
}

class idscore_t {
    public:
        constexpr static const int score_frag =  5;
        constexpr static const int score_ship = 10 * score_frag;

    private:
        int m_id;
        int m_score;

    public:
        idscore_t(const int id)
        : m_id(id), m_score(0) {}

        constexpr int id() const noexcept { return m_id; }
        constexpr int score() const noexcept { return m_score; }
        void add_score(int diff) noexcept { m_score += diff; }
        void reset() noexcept { m_score = 0; }
        bool operator==(const idscore_t& o) const noexcept = default;
};
static idscore_t world_id(0);

class star_t {
    public:
        const float r0;
        const float g0_env, g0_ships; // [m/s^2]
        pixel::f2::disk_t body;
        float dr_dir = 1;

        star_t(const pixel::f2::point_t& p0, const float r, const float g_env, const float g_ships)
        : r0(r), g0_env(g_env), g0_ships(g_ships), body(p0, r) {}

        bool tick(const float dt) noexcept {
            const float dr_min = r0 * 0.95f;
            const float dr_max = r0 * 1.05f;
            const float r = body.radius + r0 * 0.1f * dt * dr_dir;
            if( r <= dr_min ) {
                dr_dir = 1;
                body.radius = dr_min;
            } else if( r >= dr_max ) {
                dr_dir = -1;
                body.radius = dr_max;
            } else {
                body.radius = r;
            }
            return true;
        }
        void draw() {
            body.draw(true);
        }

        /** Returns star's environmental gravity [m/s^2] impact on given position. */
        pixel::f2::vec_t gravity_env(const pixel::f2::point_t& p) {
            return gravity(p, g0_env);
        }
        /** Returns star's spaceship's gravity [m/s^2] impact on given position. */
        pixel::f2::vec_t gravity_ships(const pixel::f2::point_t& p) {
            return gravity(p, g0_ships);
        }
        /** Returns star's gravity [m/s^2] impact on given position. */
        pixel::f2::vec_t gravity(const pixel::f2::point_t& p, const float g0) {
            pixel::f2::vec_t v_d = body.center - p;
            const float d = v_d.length();
            if( is_zero(d) ) {
                return pixel::f2::vec_t();
            } else {
                // v.normalize() -> v / d
                // return v.normalize() * ( g0 / ( d * d ) );
                return ( v_d /= d ) *= ( g0 / ( d * d ) ); // same as above but reusing vector 'v'
            }
        }

        bool hit(const pixel::f2::point_t& c) const noexcept {
            return (c - body.center).length() <= body.radius;
        }
};

typedef std::shared_ptr<star_t> star_ref_t;

star_ref_t sun;

constexpr bool validNumberChar(char c) noexcept {
    return ('0' <= c && c <= '9') ||
           '.' == c || 0 == c || '-' == c;
}


class fragment_t : public pixel::f2::linestrip_t {
    public:
        pixel::f2::vec_t velocity; // [m/s]
        float rotation_velocity; // [angle_radians/s]
        bool leave_on_screen_exit;

        /**
         *
         * @param center center position
         * @param angle angle in radians
         * @param v velocity in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        fragment_t(const pixel::f2::point_t& center,
                   const float angle, const float v, const float rot_v, bool leave_on_screen_exit_) noexcept
        : linestrip_t(center, angle),
          velocity( pixel::f2::vec_t::from_length_angle(v, angle) ),
          rotation_velocity(rot_v), leave_on_screen_exit(leave_on_screen_exit_)
        { }
        
        bool operator==(const fragment_t& o) const noexcept {
            return p_list == o.p_list && p_center == o.p_center && o.leave_on_screen_exit == leave_on_screen_exit && o.rotation_velocity == rotation_velocity && o.velocity == velocity &&
                matchAngle(o.dir_angle, dir_angle);
        }
        
        /**
         *
         * @param center center position
         * @param v velocity vector in meter per seconds
         * @param rot_v rotation velocity in radians per seconds
         */
        fragment_t(const pixel::f2::point_t& center,
                const pixel::f2::vec_t& v, const float rot_v, bool leave_on_screen_exit_) noexcept
        : linestrip_t(center, v.angle()),
          velocity(v),
          rotation_velocity(rot_v), leave_on_screen_exit(leave_on_screen_exit_)
        { }

        bool tick(const float dt) noexcept override {
            pixel::f2::vec_t g = sun->gravity_env(p_center);
            velocity += g * dt;
            move(velocity * dt);
            rotate(rotation_velocity * dt);
            if( p_center.x < pixel::cart_coord.min_x() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(pixel::cart_coord.max_x()-p_center.x, 0.0f);
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(pixel::cart_coord.min_x()-p_center.x, 0.0f);
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(0.0f, pixel::cart_coord.max_y()-p_center.y);
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                if( leave_on_screen_exit ) {
                    return false;
                }
                move(0.0f, pixel::cart_coord.min_y()-p_center.y);
            }
            return true;
        }

        void draw() const noexcept override {
            linestrip_t::draw();
            if( debug_gfx ) {
                pixel::set_pixel_color(rgba_yellow);
                pixel::f2::lineseg_t::draw(p_center, p_center+velocity);
                pixel::set_pixel_color(rgba_white);
            }
        }
    
        std::string toString() const noexcept override {
            std::string result = "[";
            for(const pixel::f2::point_t& point : p_list) {
                result.append(to_string("%f %f;", point.x, point.y));
            }
            result.append(to_string("center %f %f; leave_on_exit %d; rotationVelo %f; Velo %f %f; Angle %f", 
                            p_center.x, p_center.y, leave_on_screen_exit, rotation_velocity, velocity.x, velocity.y, dir_angle));
            result.append("]");
            return result;
        }

        FloatBin toBin(FloatBin& sink) const noexcept {
            sink.put(p_list.size() * 2 + 5); // NOLINT
            for(const pixel::f2::point_t& point : p_list) {
                sink.put(point.x);
                sink.put(point.y);
            }
            sink.put(leave_on_screen_exit);
            sink.put(rotation_velocity);
            sink.put(velocity.x);
            sink.put(velocity.y);
            sink.put(dir_angle);
            return sink;
        }

        bool fromString(char * code, char * end, char* &endPos) noexcept {
            char *pos = code;
            double v;
            int i=0;
            char *old_pos = nullptr;
            bool done = false;
            float x = 0;
            float y = 0;
            bool points = true;
            int pointEnd;

            while (*pos!='[' && pos < end) ++pos;
            if (pos < end) {
                ++pos;
            } else {
                endPos = pos;
                return false;
            }

            for(;!done && pos < end && old_pos != pos; ++i) {
                if(*(pos+1) == 'c') {
                    points = false;
                    pointEnd = i;
                }
                while (!validNumberChar(*pos) && pos < end) ++pos;
                old_pos = pos;
                v = strtod(pos, &pos);
                if(points) {
                    if(i % 2 == 0) {
                        x = v; // NOLINT
                    } else {
                        y = v; // NOLINT
                        p_list.emplace_back(x, y);
                    }
                } else {
                    int a = i - pointEnd;
                    static float CenterX;
                    static float VeloX;
                    const float v_f = float(v);
                    switch (a) {
                        case 0: CenterX = v_f; break;
                        case 1: p_center = pixel::f2::point_t(CenterX, v_f); break;
                        case 2: leave_on_screen_exit = v_f == 1; break;
                        case 3: rotation_velocity = v_f; break;
                        case 4: VeloX = v_f; break;
                        case 5: velocity = pixel::f2::vec_t(VeloX, v_f); break;
                        case 6: dir_angle = v_f; endPos = pos; done = true; break;
                        default:
                            endPos = pos; 
                            return false;
                    }
                }
            }
            while (*pos!=']' && pos < end) ++pos;
            if (pos < end) {
                ++pos;
                endPos = pos;
                return true;
            } else {
                endPos = pos;
                return false;
            }
        }
        
        bool fromBin(FloatBin& bb) noexcept {
            size_t size = size_t(bb.get());
            if(bb.size() < size) {
                return false;
            }
            this->p_list.clear();
            for(size_t i = 0; i <= size-5; ++i) {
                p_list.emplace_back(bb.get(), bb.get());
            }
            leave_on_screen_exit = bb.get(); // NOLINT
            rotation_velocity  = bb.get();bb.put(rotation_velocity);
            bb.put(velocity.x);
            bb.put(velocity.y);
            bb.put(dir_angle);
            return true;
        }
};
typedef std::shared_ptr<fragment_t> fragment_ref_t;

std::vector<fragment_ref_t> fragments;

void make_fragments(std::vector<fragment_ref_t>& dest,
                   const pixel::f2::linestrip_ref_t& ls, const float v, const float rot_v)
{
    if( ls->p_list.size() <= 3+1 ) {
        return; // drop simple fragment
    }
    pixel::f2::point_t p0 = ls->p_list[0];
    for(size_t i=1; i<ls->p_list.size(); ++i) {
        const pixel::f2::point_t& pc = ls->p_center;
        const pixel::f2::point_t& p1 = ls->p_list[i];
        // const double area = std::abs( pixel::f2::tri_area(pc, p0, p1) );
        // if( area >= 0.5f ) {
        {
            const pixel::f2::point_t p_v = p0 + ( p1 - p0 ) / 2.0f;
            const pixel::f2::vec_t v_v0 = ( p_v - pc ).normalize() * v;
            fragment_ref_t f = std::make_shared<fragment_t>(pc, v_v0, rot_v, true);
            f->p_list.push_back(pc);
            f->p_list.push_back(p0);
            f->p_list.push_back(p1);
            f->p_list.push_back(pc);
            f->normalize_center();
            dest.push_back(f);
        }
        p0 = p1;
    }
}

fragment_ref_t make_asteroid(const pixel::f2::point_t& center, float height,
                             const float angle,
                             const float velocity,
                             const float rot_velocity,
                             const float jitter=1.0f/8.0f) noexcept
{
    const float max_height = 6.0f*height;
    const float min_height = 1.0f*height;
    height = std::max(min_height, std::min(height, 6*max_height));
    fragment_ref_t lf = std::make_shared<fragment_t>(center, angle, velocity, rot_velocity, false);

    const float w = height;
    const float j = height * jitter;

    // a
    pixel::f2::point_t p = center;
    p.x += -w/4.0f + j;
    p.y +=  height/2.0f - j;
    lf->p_list.push_back(p);
    pixel::f2::point_t a = p;

    // lf->lala = 4;

    // b
    p.x += w/2.0f + j;
    p.y += j;
    lf->p_list.push_back(p);

    // c
    p.x +=  w/4.0f - j;
    p.y += -height/4.0f + j;
    lf->p_list.push_back(p);

    // d
    p.x +=  j;
    p.y += -height/2.0f + j;
    lf->p_list.push_back(p);

    // e
    p.x += -w/4.0f + j;
    p.y += -height/4.0f + j;
    lf->p_list.push_back(p);

    // f
    p.x += -w/2.0f;
    p.y += -j-j;
    lf->p_list.push_back(p);

    // g
    p.x += -w/4.0f - j;
    p.y += height/4.0f - j;
    lf->p_list.push_back(p);

    // height
    p.x += j;
    p.y += height/2.0f - j;
    lf->p_list.push_back(p);

    // a
    lf->p_list.push_back(a);

    lf->normalize_center();
    return lf;
}

class peng_t {
  private:
    idscore_t* m_owner;
    float m_fuse; // [s]
    constexpr static float fuse2_max = 30_s;
    float m_fuse2 = fuse2_max;

    bool hits_fragment() noexcept {
        bool hit = false;
        std::vector<fragment_ref_t> new_fragments;
        for(auto it = fragments.begin(); it != fragments.end(); ) {
            fragment_ref_t f = *it;
            if( m_peng.box().intersects(f->box()) ) {
                m_owner->add_score(idscore_t::score_frag);
                hit = true;
                it = fragments.erase(it);
                make_fragments(new_fragments, f,
                               f->velocity.length() + m_velo.length()/4,
                               f->rotation_velocity*2.0f);
            } else {
                ++it;
            }
        }
        fragments.insert(fragments.end(), new_fragments.begin(), new_fragments.end());
        return hit;
    }

  public:
    pixel::f2::vec_t m_velo; // [m/s]
    pixel::f2::rect_t m_peng;

    peng_t(idscore_t* owner,
           const pixel::f2::point_t& p0, const float diag, const pixel::f2::vec_t& v,
           const float fuse = 0.25f) noexcept
    : m_owner(owner), m_fuse(fuse), m_velo( v ),
      m_peng(p0 + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, v.angle())
    { }
    
    bool operator==(const peng_t& o) const noexcept = default;

    idscore_t& owner() noexcept { return *m_owner; }

    bool tick(const float dt) noexcept {
        if( !m_peng.on_screen() ) {
            return false;
        }
        if(!m_velo.is_zero()){
            pixel::f2::vec_t g = sun->gravity_env(m_peng.p_center);
            m_velo += g * dt;
            m_peng.move( m_velo * dt );
        }
        if( armed() ) {
            m_fuse2 -= dt;
        }
        m_peng.rotate(std::numbers::pi_v<float> * dt); // 180 degrees (180_deg)
        m_fuse = std::max(0.0f, m_fuse - dt);
        return m_fuse2 > 0 && !sun->hit( m_peng.p_center ) &&
               !hits_fragment();
    }
    bool armed() const noexcept { return is_zero(m_fuse); }
    float fuse() const noexcept { return m_fuse; }
    float& fuse() noexcept { return m_fuse; }
    float fuse2() const noexcept { return m_fuse2; }
    float& fuse2() noexcept { return m_fuse2; }
    pixel::f2::rect_t body() noexcept { return m_peng; }
    float diag() const noexcept { return std::abs((m_peng.m_tl - m_peng.m_tr).length()); }

    void draw() const noexcept {
        if( !armed() ) {
            pixel::set_pixel_color(rgba_green);
        } else {
            const float pct = m_fuse2 / fuse2_max;
            pixel::set_pixel_color4f(1.0f, pct, pct, 1.0f);
        }
        m_peng.draw(false);
        pixel::set_pixel_color(rgba_white);
    }

    std::string toString() const noexcept {
        std::string result = to_string("[ Velo %f %f, Fuse %f, Fuse2 %f, ID %d, Score %d, Diag %f, Center %f %f, Angle %f]", 
        m_velo.x, m_velo.y, m_fuse, m_fuse2, m_owner->id(), m_owner->score(), diag(), m_peng.p_center.x, m_peng.p_center.y, m_peng.dir_angle);
        return result;
    }
    
    FloatBin& toBin(FloatBin& sink) const noexcept {
        sink.put(10.0f);
        sink.put(m_velo.x);
        sink.put(m_velo.y);
        sink.put(m_fuse);
        sink.put(m_fuse2);
        sink.put(m_owner->id()); // NOLINT
        sink.put(m_owner->score()); // NOLINT
        sink.put(diag());
        sink.put(m_peng.p_center.x);
        sink.put(m_peng.p_center.y);
        sink.put(m_peng.dir_angle);
        return sink;
    }
    
    bool fromString(char * code, char * end, char* &endPos) {
        (void)endPos;
        char *pos = code;
        double v;
        int i=0;
        char *old_pos = nullptr;
        bool done = false;

        while (*pos!='[' && pos < end) ++pos;
        if (pos < end) {
            ++pos;
        } else {
            endPos = pos;
            return false;
        }
            
        for(;!done && pos < end && old_pos != pos; ++i) {
            while (!validNumberChar(*pos) && pos < end) ++pos;
            old_pos = pos;
            v = strtod(pos, &pos);
            static float CenterX;
            static float VeloX;
            static int id;
            static float diag;
            const float v_f = float(v);
            switch (i) {
                case  0: VeloX = v_f; break;
                case  1: m_velo = pixel::f2::vec_t(VeloX, v_f); break;
                case  2: fuse() = v_f; break;
                case  3: fuse2() = v_f; break;
                case  4: id = int(v_f); break;
                case  5: { idscore_t idscore = idscore_t(id); idscore.add_score(int(v_f)); m_owner = &idscore; break; }
                case  6: diag = v_f; break;
                case  7: CenterX = v_f; break;
                case  8: {
                    pixel::f2::point_t center(CenterX, v_f);
                    body() = pixel::f2::rect_t(center + pixel::f2::point_t(-diag/2, +diag/2), diag, diag, 0);
                    break;
                }
                case 9: body().rotate(v_f); endPos = pos; done = true; break;
                default:
                    endPos = pos; 
                    return false;
            }
        }
        while (*pos!=']' && pos < end) ++pos;
        if (pos < end) {
            ++pos;
            endPos = pos;
            return true;
        } else {
            endPos = pos;
            return false;
        }
    }
    
    bool fromBin(FloatBin& bb) noexcept {
        if(bb.remaining() < size_t(bb.get())) {
            return false;
        }
        m_velo.x = bb.get();
        m_velo.y = bb.get();
        m_fuse = bb.get();
        m_fuse2 = bb.get();
        if(m_owner == nullptr) {
            m_owner = (idscore_t*)malloc(sizeof(idscore_t));
        }
        *m_owner = idscore_t(bb.get()); // NOLINT
        m_owner->add_score(bb.get()); // NOLINT
        const float diag = bb.get();
        const pixel::f2::point_t center = pixel::f2::point_t(bb.get(), bb.get());
        m_peng = pixel::f2::rect_t(center + pixel::f2::vec_t(-diag/2, diag/2), diag, diag, 0);
        m_peng.rotate(bb.get());
        return true;
    }
    
    void changeSpeed(float a){
        m_velo *= a;
    }

    bool intersection(const peng_t& o) const {
        return m_peng.intersects(o.m_peng);
    }
};

std::vector<peng_t> pengs;

class player_t;

class spaceship_t : public pixel::f2::linestrip_t {
    public:
        constexpr static const float height = spaceship_height; // [m]
        constexpr static const float vel_step = 5.0f; // [m/s]
        constexpr static const float vel_max = 100.0f + vel_step; // [m/s]
        constexpr static const float rot_step = 180.0f; // [ang-degrees / s]

        constexpr static const float peng_diag = 0.15f*height;
        constexpr static const float peng_velo_0 = vel_max / 2;
        constexpr static const int peng_inventory_max = 4000;
        constexpr static const int mine_inventory_max = 2000;

        constexpr static const float shield_radius = spaceship_height * 0.9f;
        constexpr static const float shield_time_max = 10; // [s]

    private:
        idscore_t &m_owner;
        float m_shield_time;
        bool m_shield;

        friend player_t;
        
        bool hits_fragment(const pixel::f2::aabbox_t& box) noexcept {
            bool hit = false;
            std::vector<fragment_ref_t> new_fragments;
            for(auto it = fragments.begin(); it != fragments.end(); ) {
                fragment_ref_t f = *it;
                if( box.intersects(f->box()) ) {
                    if( !m_shield ) {
                        m_owner.add_score(-idscore_t::score_ship);
                    }
                    hit = true;
                    it = fragments.erase(it);
                    make_fragments(new_fragments, f,
                            f->velocity.length() + velocity.length(),
                            f->rotation_velocity*2.0f);
                } else {
                    ++it;
                }
            }
            fragments.insert(fragments.end(), new_fragments.begin(), new_fragments.end());
            return hit;
        }
        bool hits_peng(const pixel::f2::aabbox_t& box) noexcept {
            bool hit = false;
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if( p.armed() && box.intersects(p.m_peng.box()) ) {
                    hit = true;
                    it = pengs.erase(it);
                    if( !m_shield ) {
                        if( m_owner.id() == p.owner().id() ) {
                            m_owner.add_score(-idscore_t::score_ship);
                        } else {
                            p.owner().add_score(idscore_t::score_ship);
                        }
                    }
                } else {
                    ++it;
                }
            }
            return hit;
        }

    public:
        pixel::f2::disk_t shield_body;
        pixel::f2::vec_t velocity; // [m/s]
        int peng_inventory;
        int mine_inventory;

        spaceship_t(idscore_t& owner,
                    const pixel::f2::point_t& center, const float angle) noexcept
        : linestrip_t(center, angle), m_owner(owner),
          m_shield_time(shield_time_max), m_shield(false), shield_body(p_center, shield_radius), 
          velocity(), peng_inventory(peng_inventory_max), mine_inventory(mine_inventory_max) {}

        void finalize_make() noexcept {
            normalize_center();
            shield_body.center = p_center;
        }

        bool operator==(const spaceship_t& o) const noexcept {
            return o.m_shield == m_shield && o.m_shield_time == m_shield_time && o.shield_body == shield_body && o.m_owner == m_owner &&
                o.mine_inventory == mine_inventory && o.peng_inventory == peng_inventory && o.velocity == velocity &&
                matchAngle(o.dir_angle, dir_angle) && o.p_center == p_center /* FIXME && o.p_list == p_list */;
        }
        
        std::string checkEqual(const spaceship_t& o) const noexcept {
            if(*this == o) {
                return "OK";
            } else {
                std::string result;
                result += to_string("shield %d, ", o.m_shield == m_shield);
                result += to_string("shield time %d, ", o.m_shield_time == m_shield_time);
                result += to_string("shield body %d(o %s, this %s), ", o.shield_body == shield_body, o.shield_body.toString().c_str(), shield_body.toString().c_str());
                result += to_string("owner %d, ", o.m_owner == m_owner);
                result += to_string("mines %d, ", o.mine_inventory == mine_inventory);
                result += to_string("pengs %d, ", o.peng_inventory == peng_inventory);
                result += to_string("velo %d, ", o.velocity == velocity);
                result += to_string("angle %d(o %.10f, this %.10f), ", matchAngle(o.dir_angle, dir_angle), o.dir_angle, dir_angle);
                result += to_string("center %d(this %s, o %s), ", o.p_center == p_center, p_center.toString().c_str(), o.p_center.toString().c_str());
                result += to_string("p_list %d(size: o %zu, this %zu; points: o [", o.p_list == p_list, o.p_list.size(), p_list.size());
                for(pixel::f2::point_t p : o.p_list) {
                    result += p.toString() + ", ";
                }
                result += "] this [";
                for(pixel::f2::point_t p : p_list) {
                    result += p.toString() + ", ";
                }
                result += "]";
                return result;
            }
        }
        void peng() noexcept {
            if(peng_inventory > 0){
                pixel::f2::point_t p0;
                // adjust start posision to geometric ship model
                if(m_owner.id() == player_id_3){
                    p0 = p_list[4];
                } else {
                    p0 = p_list[0];
                }
                pixel::f2::vec_t v_p = velocity + pixel::f2::vec_t::from_length_angle(peng_velo_0, dir_angle);
                pengs.emplace_back(&m_owner, p0, peng_diag, v_p);
                --peng_inventory;
            }
        }

        void mine() noexcept {
            if(mine_inventory <= 0){
                return;
            }
            if( true ) {
                pengs.emplace_back(&m_owner,
                    pixel::f2::point_t(p_center.x, p_center.y-peng_diag)-
                    5*velocity.copy().normalize(),
                    peng_diag, pixel::f2::vec_t(), 2);
            } else {
                pengs.emplace_back(&m_owner,
                    p_center, peng_diag, -velocity, 2);
            }
            --mine_inventory;
        }

        void velo_up(const float dv = vel_step) noexcept {
            pixel::f2::vec_t v = velocity;
            v += pixel::f2::vec_t::from_length_angle(dv, dir_angle);
            if( v.length() < vel_max ) {
                velocity = v;
            }
        }
        
        void rotate_adeg(const float da_adeg) {
            rotate(adeg_to_rad(da_adeg));
        }

        constexpr bool shield() const noexcept { return m_shield; }
        constexpr bool& shield() noexcept { return m_shield; }
        constexpr float shield_time() const noexcept { return m_shield_time; }
        constexpr float& shield_time() noexcept { return m_shield_time; }

        void toggle_shield() noexcept {
            set_shield(!m_shield);
        }
        void set_shield(bool v) noexcept {
            if( m_shield_time <= 0 ) {
                m_shield = false;
            } else {
                m_shield = v;
            }
        }

        pixel::f2::aabbox_t box() const noexcept override {
            if( m_shield ) {
                return shield_body.box();
            } else {
                return linestrip_t::box();
            }
        }
        void move(const pixel::f2::point_t& d) noexcept override {
            linestrip_t::move(d);
            if( p_center.x < pixel::cart_coord.min_x() ) {
                linestrip_t::move(pixel::cart_coord.max_x()-p_center.x, 0.0f);
            }
            if( p_center.x > pixel::cart_coord.max_x() ) {
                linestrip_t::move(pixel::cart_coord.min_x()-p_center.x, 0.0f);
            }
            if( p_center.y < pixel::cart_coord.min_y() ) {
                linestrip_t::move(0.0f, pixel::cart_coord.max_y()-p_center.y);
            }
            if( p_center.y > pixel::cart_coord.max_y() ) {
                linestrip_t::move(0.0f, pixel::cart_coord.min_y()-p_center.y);
            }
            shield_body.center = p_center;
        }
        void move2(const pixel::f2::point_t& d) noexcept {
            linestrip_t::move(d);
            shield_body.center = p_center;
        }
        bool tick(const float dt) noexcept override {
            pixel::f2::vec_t g = sun->gravity_ships(p_center);
            velocity += g * dt;

            move(velocity * dt);

            if( m_shield ) {
                m_shield_time -= dt;
                if( m_shield_time <= 0 ) {
                    m_shield = false;
                    m_shield_time = 0;
                }
            }
            if( sun->hit(p_center) ) {
                m_owner.add_score(-m_owner.score_ship);
                return false;
            }
            pixel::f2::aabbox_t b = box();
            return ( !hits_fragment(b) && !hits_peng(b) ) || m_shield;
        }

        void set_orbit_velocity() noexcept {
            velocity = orbit_velocity();
        }
        pixel::f2::vec_t orbit_velocity() const noexcept {
            // g = v^2 / d
            const float d = ( sun->body.center - p_center ).length();
            pixel::f2::vec_t g = sun->gravity_ships(p_center);
            const float v0 = std::sqrt( g.length() * d );
            g.normalize().rotate(-M_PI_2) *= v0; // becomes velocity
            if( g.angle(velocity) > M_PI_2 ) {
                g.rotate(M_PI); // rotate to velocity direction
            }
            return g;
        }

        void draw() const noexcept override {
            //pixel::set_pixel_color(rgba_white);
            if( m_shield ) {
                shield_body.draw(false);
            }
            linestrip_t::draw();
            if( show_ship_velo ) {
                pixel::set_pixel_color(rgba_yellow);
                pixel::f2::lineseg_t::draw(p_center, p_center+velocity);
                pixel::set_pixel_color(rgba_red);
                pixel::f2::vec_t v_o = orbit_velocity();
                pixel::f2::lineseg_t::draw(p_center, p_center+v_o);
                // pixel::set_pixel_color(rgba_green);
                // pixel::f2::disk_t o(sun->body.center, (sun->body.center - p_center).length());
                // o.draw(false);
                pixel::set_pixel_color(rgba_white);
            }
        }
        std::string toString() const noexcept override {
            std::string result;
            result.append(to_string("Ship [ShieldTime %f, shield %d, Mines %d, Pengs %d, Velo %f %f, Center %f %f, Angle %f]", 
                shield_time(), shield(), mine_inventory, peng_inventory, velocity.x, velocity.y, p_center.x, p_center.y, dir_angle));
            return result;
        }
        FloatBin& toBin(FloatBin& sink) const noexcept {
            sink.put(m_shield_time);
            sink.put(m_shield);
            sink.put(mine_inventory); // NOLINT
            sink.put(peng_inventory); // NOLINT
            sink.put(velocity.x);
            sink.put(velocity.y);
            sink.put(p_center.x);
            sink.put(p_center.y);
            sink.put(dir_angle);
            return sink;
        }
};
typedef std::shared_ptr<spaceship_t> spaceship_ref_t;
std::vector<spaceship_ref_t> spaceship;

/**
 * Unrotated:
 *
 *       (a)
 *      /  \
 *     / (c)\
 *    /  / \ \
 *   (d)/   \(b)
 */
spaceship_ref_t make_spaceship1(idscore_t &owner, const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(owner, m, 90_deg);

    const float width = 4.0f/5.0f * h;;

    // a
    pixel::f2::point_t p = m;
    p.y += h/2.0f;
    lf->p_list.push_back(p);

    // lf->lala = 4;

    // b
    p.y -= h;
    p.x += width/2.0f;
    lf->p_list.push_back(p);

    // c
    lf->p_list.push_back(m);

    // d
    p.x -= width;
    lf->p_list.push_back(p);

    p = m;
    p.y += h/2.0f;
    lf->p_list.push_back(p);
    lf->finalize_make();
    return lf;
}

/**
 * Unrotated:
 *
 *      (a)
 *      | |
 *    | | | |
 *    | | | |
 *    |=|c|=|
 *
 */
spaceship_ref_t make_spaceship2(idscore_t& owner, const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(owner, m, 90_deg);

    const float w = 4.0f / 5.0f * h;
    const float w_s = w / 4.0f;
    const float h_s = h / 5.0f;
    pixel::f2::point_t p = m;

    p.x -= 0.25f * w_s;
    p.y += h / 2;
    lf->p_list.push_back(p);

    p.y -= 4 * h_s;
    lf->p_list.push_back(p);

    p.x -= ( 2.0f - 0.25f ) * w_s;
    lf->p_list.push_back(p);

    p.y += 2 * h_s;
    lf->p_list.push_back(p);

    p.y -= 3 * h_s;
    lf->p_list.push_back(p);

    p.y += 1 * h_s;
    lf->p_list.push_back(p);

    p.x += w;
    lf->p_list.push_back(p);

    p.y -= 1 * h_s;
    lf->p_list.push_back(p);

    p.y += 3 * h_s;
    lf->p_list.push_back(p);

    p.y -= 2 * h_s;
    lf->p_list.push_back(p);

    p.x -= ( 2.0f - 0.25f ) * w_s;
    lf->p_list.push_back(p);

    p.y += 4 * h_s;
    lf->p_list.push_back(p);

    p.x -= 0.50f * w_s;
    lf->p_list.push_back(p);

    lf->finalize_make();
    return lf;
}

/**
 * Unrotated:
 *
 *    /\
 *   /  \
 *|  |  |  |
 *----------
 */
spaceship_ref_t make_spaceship3(idscore_t& owner, const pixel::f2::point_t& m, const float h=spaceship_t::height) noexcept
{
    spaceship_ref_t lf = std::make_shared<spaceship_t>(owner, m, 90_deg);

    const float w = 4.0f / 5.0f * h;
    pixel::f2::point_t p = {m.x - w / 2, m.y - h / 2};

    lf->p_list.push_back(p);

    p.y -= h / 2;
    lf->p_list.push_back(p);

    p.x += w / 3;
    lf->p_list.push_back(p);

    p.y += h / 2;
    lf->p_list.push_back(p);

    p.y += h / 2;
    p.x += w / 6;
    lf->p_list.push_back(p);

    p.x += w / 6;
    p.y -= h / 2;
    lf->p_list.push_back(p);

    p.y -= h / 2;
    lf->p_list.push_back(p);

    p.x -= w / 3;
    lf->p_list.push_back(p);

    p.x += w / 3 * 2;
    lf->p_list.push_back(p);

    p.y += h / 2;
    lf->p_list.push_back(p);

    lf->finalize_make();
    return lf;
}

void reset_asteroids(int count) {
    fragments.clear();
    for(int i = 0; i < count; ++i) {
        const float height_h = spaceship_t::height*2.0f;
        const float height = height_h + height_h*next_rnd();
        const float angle = adeg_to_rad(next_rnd() * 360.0f);
        const float velocity = 10.0f + next_rnd() * 10.0f; // m/s
        const float rot_velocity = ( 15_deg +
                                     next_rnd() * 15_deg )
                                   * ( i%2 == 0 ? 1.0f : -1.0f ); // angle/s
        const float jitter = 1.0f / ( 4.0f + 4.0f * next_rnd() );
        pixel::f2::point_t p0(pixel::cart_coord.min_x()+(pixel::cart_coord.width()*next_rnd()),
                              i%2 == 0 ? pixel::cart_coord.min_y()+height/2 : pixel::cart_coord.max_y()-height/2);
        fragment_ref_t asteroid1 = make_asteroid(p0, height,
                angle, velocity, rot_velocity, jitter );
        fragments.push_back(asteroid1);
    }

}

class player_t : public idscore_t {
    private:
        constexpr static const pixel::f2::point_t p0_ss1 = {  6 * spaceship_height,  6 * spaceship_height };
        constexpr static const pixel::f2::point_t p0_ss2 = { -6 * spaceship_height, -6 * spaceship_height };
        constexpr static const pixel::f2::point_t p0_ss3 = {  6 * spaceship_height, -6 * spaceship_height };
        constexpr static const float disable_cloak_time_per_shot = 2.0f;
        constexpr static const float default_respawn_timer = 5.0f; // [s]
        bool cloak_shot = false;
        float cloak_shot_timer = 0.0f;
        float m_respawn_timer = 0.0f;
        spaceship_ref_t m_ship;
        bool m_cloak;

        void ship_dtor() noexcept {
            make_fragments(fragments, m_ship, m_ship->velocity.length() + spaceship_t::vel_step, 0.003f);
            m_ship = nullptr;
            m_respawn_timer = default_respawn_timer; // [s]
        }

        void respawn_ship() noexcept {
            m_cloak = false;
            m_respawn_timer = 0;
            if( player_id_1 == id() ) {
                m_ship = make_spaceship1(idscore(), p0_ss1);
            } else if(player_id_2 == id()){
                m_ship = make_spaceship2(idscore(), p0_ss2);
            } else {
                m_ship = make_spaceship3(idscore(), p0_ss3);
            }
            m_ship->set_orbit_velocity();
        }
        
        void set_idScore(int new_id, int score) noexcept { idscore_t::operator=(idscore_t(new_id)); idscore_t::add_score(score); }

        int m_sessionSocket = -1;
    public:
        idscore_t& idscore() noexcept { return *this; }
        const idscore_t& idscore() const noexcept { return *this; }
    
        int sessionSocket() const noexcept { return m_sessionSocket; }
        
        bool serviceGame(int port) {
            m_sessionSocket = serverWaitForClient(port);
            return connected() || io::ioEnv->netInValid();
        }
        bool connectToServer(std::string& name, int port) {
            m_sessionSocket = clientConnectToServer(name, port);
            return connected() || io::ioEnv->netInValid();
        }
        bool connected() const noexcept { return 0 <= sessionSocket(); }    
        void disconnect() {
            if( connected() ) {
                ::close(m_sessionSocket);
                m_sessionSocket = -1;
            }
        }
        
        static void collision(player_t& player1, player_t& player2) noexcept {
            spaceship_ref_t& ship1 = player1.m_ship;
            spaceship_ref_t& ship2 = player2.m_ship;
            if( nullptr != ship1 && nullptr != ship2 && ship1->intersects(*ship2)) {
                if( !ship2->shield() && !ship1->shield() ) {
                    player1.ship_dtor();
                    player2.ship_dtor();
                } else if( !ship2->shield() && ship1->shield() ) {
                    player2.ship_dtor();
                    player2.add_score(-score_ship);
                } else if( ship2->shield() && !ship1->shield() ) {
                    player1.add_score(-score_ship);
                    player1.ship_dtor();
                } else if( ship2->shield() && ship1->shield() ) {
                    /**
                    pixel::f2::vec_t cross_normal;
                    pixel::f2::point_t cross_point;
                    pixel::f2::vec_t tangente_r = pr.m_ship->velocity.normal_ccw();
                    pixel::f2::vec_t tangente_l = pl.m_ship->velocity.normal_ccw();
                            pl.m_ship->velocity.normal_ccw();
                    // pr.m_ship->shield_body.intersection(pr.m_ship->velocity, cross_normal, cross_point, tangente_r);
                    // pl.m_ship->shield_body.intersection(pl.m_ship->velocity, cross_normal, cross_point, tangente_l);
                     */
                    {
                        pixel::f2::vec_t cross_normal, reflect_out;
                        pixel::f2::point_t cross_point;
                        pixel::f2::vec_t v_norm(ship2->velocity);
                        v_norm.normalize();
                        pixel::f2::point_t p0 = ship2->p_center - v_norm * ship2->shield_radius;
                        pixel::f2::point_t p1 = ship2->p_center + v_norm * ship2->shield_radius;
                        pixel::f2::lineseg_t l12(p0, p1);
                        if( ship1->shield_body.intersection(reflect_out, cross_normal, cross_point, l12) ) {
                            std::cout << "XXX3.0: pr.c " << ship2->p_center
                                      << ", v " << ship2->velocity
                                      << ", cross " << cross_point
                                      << ", r-out " << reflect_out << std::endl;
                            {
                                pixel::f2::vec_t d = reflect_out; //  - ship2->p_center;
                                ship2->move(d);
                                ship2->velocity = ship2->velocity.length() * reflect_out.normalize();
                                std::cout << "XXX3.X: pr.c " << ship2->p_center
                                          << ", d " << d
                                          << ", v " << ship2->velocity << std::endl;
                            }
                            if ( false ) {
                                pixel::f2::vec_t d = reflect_out - ship1->p_center;
                                ship1->move(d);
                                ship1->velocity = ship1->velocity.length() * d.normalize();
                            }
                            event.set_paused(false);
                        }
                    }
                    if( true ){
                        pixel::f2::vec_t cross_normal, reflect_out;
                        pixel::f2::point_t cross_point;
                        pixel::f2::vec_t v_norm(ship1->velocity);
                        v_norm.normalize();
                        pixel::f2::point_t p0 = ship1->p_center - v_norm * ship1->shield_radius;
                        pixel::f2::point_t p1 = ship1->p_center + v_norm * ship1->shield_radius;
                        pixel::f2::lineseg_t l12(p0, p1);
                        if( ship2->shield_body.intersection(reflect_out, cross_normal, cross_point, l12) ) {
                            std::cout << "XXX3.0: pr.c " << ship1->p_center
                                      << ", v " << ship2->velocity
                                      << ", cross " << cross_point
                                      << ", r-out " << reflect_out << std::endl;
                            {
                                pixel::f2::vec_t d = reflect_out; //  - ship2->p_center;
                                ship1->move(d);
                                ship1->velocity = ship1->velocity.length() * reflect_out.normalize();
                                std::cout << "XXX3.X: pr.c " << ship2->p_center
                                          << ", d " << d
                                          << ", v " << ship1->velocity << std::endl;
                            }
                            if ( false ) {
                                pixel::f2::vec_t d = reflect_out - ship2->p_center;
                                ship2->move(d);
                                ship2->velocity = ship1->velocity.length() * d.normalize();
                            }
                            event.set_paused(false);
                        }
                    }
                }
            }
        }

        player_t(const player_t&)            = default;
        player_t(player_t&&)                 = default;
        player_t& operator=(const player_t&) = default;
        player_t& operator=(player_t&&)      = default;
        player_t(const int id) noexcept
        : idscore_t(id), m_ship(nullptr) { respawn_ship(); }
        bool operator==(const player_t& o) const noexcept {
            return m_cloak == o.m_cloak && cloak_shot == o.cloak_shot && m_respawn_timer == o.m_respawn_timer && 
                ( (!m_ship && !o.m_ship) || (m_ship && o.m_ship && *m_ship == *o.m_ship) ) &&
                cloak_shot_timer == o.cloak_shot_timer && m_sessionSocket == o.m_sessionSocket &&
                o.id() == id() && o.score() == score();
        }
    
        std::string checkEqual(const player_t& o) const noexcept {
            if(*this == o) {
                return "OK";
            } else {
                std::string result;
                result += to_string("cloak %d, ", m_cloak == o.m_cloak);
                result += to_string("cloak_shot %d, ", cloak_shot == o.cloak_shot );
                result += to_string("respawn timer %d, ", m_respawn_timer == o.m_respawn_timer);
                result += to_string("ship %d, ", ( (!m_ship && !o.m_ship) || (m_ship && o.m_ship && *m_ship == *o.m_ship) ) );
                result += to_string("cloak shot timer %d, ", cloak_shot_timer == o.cloak_shot_timer);
                result += to_string("sessionSocket %d, ", m_sessionSocket == o.m_sessionSocket);
                result += to_string("ID %d, ", o.id() == id());
                result += to_string("SCORE %d", o.score() == score());
                return result;
            }
        }
        
        std::string checkShipEqual(const player_t& o) const noexcept {
            return m_ship && o.m_ship ? m_ship->checkEqual(*o.m_ship) : "";
        }
        void reset(bool respawn) noexcept {
            idscore_t::reset();
            cloak_shot = false;
            cloak_shot_timer = 0.0f;
            if (respawn) {
                respawn_ship();
            } else {
                m_respawn_timer = 0.0f;
                m_ship = nullptr;
                m_cloak = false;                
            }
        }
        void reset2() noexcept {
            idscore_t::reset();
            cloak_shot = false;
            cloak_shot_timer = 0.0f;
            m_respawn_timer = 0.0f;
            m_cloak = false;                
        }
        spaceship_ref_t ship() noexcept { return m_ship; }
        bool has_ship() const noexcept { return nullptr != m_ship; }

        float velocity() const noexcept { return nullptr != m_ship ? m_ship->velocity.length() : 0; }
        
        pixel::f2::vec_t velo() const noexcept { return nullptr != m_ship ? m_ship->velocity : pixel::f2::vec_t(0, 0); }

        int peng_inventory() const noexcept { return nullptr != m_ship ? m_ship->peng_inventory : 0; }

        int mine_inventory() const noexcept { return nullptr != m_ship ? m_ship->mine_inventory : 0; }

        float shield_time() const noexcept { return nullptr != m_ship ? m_ship->shield_time() : 0; }
        
        bool shieldON() const noexcept { return nullptr != m_ship ? m_ship->shield() : false; }

        float angle() const noexcept { return nullptr != m_ship ? m_ship->dir_angle : 0; }

        bool cloak() const noexcept { return m_cloak; }

        void set_cloak( bool v ) noexcept { m_cloak = cloak_enabled && v; }

        pixel::f2::point_t center() const noexcept {
            if(m_ship != nullptr){
                return m_ship->p_center;
            } else {
                return pixel::f2::point_t(0, 0);
            }
        }

        bool tick(const float dt) noexcept {
            if( nullptr != m_ship ) {
                if( !m_ship->tick(dt) ) {
                    ship_dtor();
                }
            } else {
                if( m_respawn_timer > 0 ) {
                    m_respawn_timer -= dt;
                }
                if( 0 >= m_respawn_timer ) {
                    respawn_ship();
                }
            }
            cloak_shot_timer -= dt;
            cloak_shot = cloak_shot_timer > 0;
            return true;
        }

        void draw() const noexcept {
            if( m_ship != nullptr) {
                if(m_cloak && !cloak_shot){
                    if((id() == player_id_1 && global_is_server) || (id() == player_id_2 && !global_is_server) ){
                        pixel::set_pixel_color4f(0.5f, 0.5f, 0.5f, 0.5f);
                    } else {
                        return;
                    }
                } else {
                    pixel::set_pixel_color4f(1.0f, 1.0f, 1.0f, 1.0f);
                }
                m_ship->draw();
            }
        }

        std::string toString() const noexcept {
            std::string result;
            result.append(to_string("Player[%d, Score %d, Cloak %d, Sock %d", 
                id(), score(), m_cloak, m_sessionSocket).c_str());
            if (has_ship()) {
                result.append(", ").append(m_ship->toString());
            }                
            result.append("]");
            return result;
        }
        
        FloatBin& toBin(FloatBin& sink) const noexcept {
            sink.put(has_ship() ? 4+9 : 4);
            sink.put(id()); // NOLINT
            sink.put(score()); // NOLINT
            sink.put(m_cloak); // NOLINT
            sink.put(m_sessionSocket); // NOLINT
            if (has_ship()) {
                m_ship->toBin(sink);
            }
            return sink;
        }

        bool fromString(char * code, char * end, char* &endPos) noexcept {
            constexpr static const bool a = false;
            (void)a;
            char *pos = code;
            double v;
            int i=0;
            char *old_pos = nullptr;
            
            reset2();
            bool done = false;
            pixel::f2::point_t center;
            pixel::f2::vec_t velo;
            int id;
            int old_id = idscore_t::id();
            float shield_time;
            bool shieldsON;
            int mine_inventory;
            int peng_inventory;
            while (*pos!='[' && pos < end) ++pos;
            if (pos < end) {
                ++pos;
            } else {
                endPos = pos;
                return false;
            }
                
            for(;!done && pos < end && old_pos != pos; ++i) {
                while (!validNumberChar(*pos) && pos < end) ++pos;
                if (pos == end) {
                    endPos = pos;
                    return false;
                }
                old_pos = pos;
                v = strtod(old_pos, &pos);
                const float v_f = float(v);            
                switch (i) {
                    case  0: id = int(v_f); break;
                    case  1: set_idScore(id, int(v_f)); break;
                    case  2: set_cloak(v_f == 1); break;
                    case  3: { 
                        if(*pos == ']') {
                            m_ship = nullptr;
                            done = true;
                        }
                        break;
                    }
                    case  4: shield_time = v_f; break;
                    case  5: shieldsON = v_f == 1; break;
                    case  6: mine_inventory = int(v_f); break;
                    case  7: peng_inventory = int(v_f); break;
                    case  8: velo.x = v_f; break;
                    case  9: velo.y = v_f; break;
                    case 10: center.x = v_f; break;
                    case 11: center.y = v_f; break;
                    case 12: {
                        if(old_id == id && m_ship != nullptr) {
                            m_ship->move2(-m_ship->p_center+center);
                            m_ship->m_owner.add_score(-m_ship->m_owner.score()+idscore_t::score());
                        } else {
                            m_ship = id == player_id_1 ? make_spaceship1(idscore(), center) : make_spaceship2(idscore(), center);
                        }
                        m_ship->mine_inventory = mine_inventory;
                        m_ship->peng_inventory = peng_inventory;
                        m_ship->shield() = shieldsON;
                        m_ship->shield_time() = shield_time;
                        m_ship->velocity = velo;
                        m_ship->rotate(v_f-m_ship->dir_angle); // FIXME: accuracy
                        done = true;
                        break;
                    }
                    default: 
                        endPos = pos; 
                        return false;
                }
            }
            while (*pos!=']' && pos < end) ++pos;
            if (pos < end) {
                ++pos;
                endPos = pos;
                return true;
            } else {
                endPos = pos;
                return false;
            }
        }
        
        bool fromBin(FloatBin& bb) {
            size_t size = size_t(bb.get());
            if(bb.remaining() < size) {
                return false;
            }
            int oldID = id();
            int id = bb.get(); // NOLINT
            set_idScore(id, bb.get()); // NOLINT
            m_cloak = bb.get() > 0;
            bb.get(); // session socket
            if(size == 4+9) {
                if(oldID != id || !m_ship) {
                    m_ship = id == player_id_1 ? make_spaceship1(idscore(), {0, 0}) : make_spaceship2(idscore(), {0, 0});
                }
                m_ship->m_shield_time = bb.get();
                m_ship->m_shield = bb.get() == 1; // NOLINT
                m_ship->mine_inventory = bb.get(); // NOLINT
                m_ship->peng_inventory = bb.get(); // NOLINT
                m_ship->velocity.x = bb.get();
                m_ship->velocity.y = bb.get();
                const pixel::f2::point_t center = {bb.get(), bb.get()};
                m_ship->move2(center-m_ship->p_center);
                m_ship->rotate(bb.get()-m_ship->dir_angle);
            } else {
                m_ship = nullptr;
            }
            return true;
        }
    
        void handle_event0() noexcept {
            if( nullptr != m_ship && event.has_any_p2() ) {
                if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION1) ) {
                    m_ship->peng();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                    io::ioEnv->sendNetLine(sessionSocket(), "Action1");
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                    m_ship->set_orbit_velocity();
                    io::ioEnv->sendNetLine(sessionSocket(), "Action2");
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION3) ) {
                    set_cloak(!cloak());
                    io::ioEnv->sendNetLine(sessionSocket(), "Action3");
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION4) ) {
                    m_ship->mine();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                    io::ioEnv->sendNetLine(sessionSocket(), "Action4");
                }
            }
        }
        void handle_event02() noexcept {
            if( nullptr != m_ship && event.has_any_p2() ) {
                if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION1) ) {
                    m_ship->peng();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                    FloatBin a;
                    a.put(6+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION2) ) {
                    m_ship->set_orbit_velocity();
                    FloatBin a;
                    a.put(7+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION3) ) {
                    set_cloak(!cloak());
                    FloatBin a;
                    a.put(8+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                } else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION4) ) {
                    m_ship->mine();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                    FloatBin a;
                    a.put(9+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                }
            }
        }
        void handle_event1(const float dt /* [s] */) noexcept {
            if( nullptr != m_ship && event.has_any_p1() ) {
                if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
                    m_ship->velo_up(spaceship_t::vel_step);
                    io::ioEnv->sendNetLine(sessionSocket(), "Up");
                } else if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                    m_ship->rotate_adeg(spaceship_t::rot_step * dt);
                    io::ioEnv->sendNetLine(sessionSocket(), "Left");
                } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                    m_ship->rotate_adeg(-spaceship_t::rot_step * dt);
                    io::ioEnv->sendNetLine(sessionSocket(), "Right");
                } else {
                    m_ship->set_shield( event.pressed(pixel::input_event_type_t::P1_DOWN) );
                    if(event.pressed(pixel::input_event_type_t::P1_DOWN)) {
                        io::ioEnv->sendNetLine(sessionSocket(), "Down");
                    }
                }
            }
        }
        void handle_event12(const float dt /* [s] */) noexcept {
            if( nullptr != m_ship && event.has_any_p1() ) {
                if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
                    m_ship->velo_up(spaceship_t::vel_step);
                    FloatBin a;
                    a.put(2+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                } else if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                    m_ship->rotate_adeg(spaceship_t::rot_step * dt);
                    FloatBin a;
                    a.put(3+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                    m_ship->rotate_adeg(-spaceship_t::rot_step * dt);
                    FloatBin a;
                    a.put(4+(id()-1) * 8); // NOLINT
                    io::ioEnv->sendNetLine(sessionSocket(), a);
                } else {
                    m_ship->set_shield( event.pressed(pixel::input_event_type_t::P1_DOWN) );
                    if(event.pressed(pixel::input_event_type_t::P1_DOWN)) {
                        FloatBin a;
                        a.put(5+(id()-1) * 8); // NOLINT
                        io::ioEnv->sendNetLine(sessionSocket(), a);
                    }
                }
            }
        }
        void receive_handle_event1(const float dt /* [s] */, const std::string& message) noexcept {
            if( nullptr != m_ship ) {
                if( message == "Up" ) {
                    m_ship->velo_up(spaceship_t::vel_step);
                } else if( message == "Left") {
                    m_ship->rotate_adeg(spaceship_t::rot_step * dt);
                } else if( message == "Right" ) {
                    m_ship->rotate_adeg(-spaceship_t::rot_step * dt);
                }
                m_ship->set_shield(message == "Down");
            }
        }
        void receive_handle_event1(const float dt /* [s] */, const FloatBin& message) noexcept {
            if( nullptr != m_ship ) {
                if( message.data()[0] == 2+(id()-1) * 8) { // NOLINT
                    m_ship->velo_up(spaceship_t::vel_step);
                } else if( message.data()[0] == 3+(id()-1) * 8) { // NOLINT
                    m_ship->rotate_adeg(spaceship_t::rot_step * dt);
                } else if( message.data()[0] == 4+(id()-1) * 8 ) { // NOLINT
                    m_ship->rotate_adeg(-spaceship_t::rot_step * dt);
                }
                m_ship->set_shield(message.data()[0] == 5+(id()-1) * 8); // NOLINT
            }
        }
        void receive_handle_event0(const std::string& message) noexcept {
            if( nullptr != m_ship ) {
                if( message == "Action1" ) {
                    m_ship->peng();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                } else if( message == "Action2" ) {
                    m_ship->set_orbit_velocity();
                } else if( message == "Action3" ) {
                    set_cloak(!cloak());
                } else if( message == "Action4" ) {
                    m_ship->mine();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                }
            }
        }
        void receive_handle_event0(const FloatBin& message) noexcept {
            if( nullptr != m_ship ) {
                if( message.data()[0] == 6+(id()-1) * 8 ) {  // NOLINT
                    m_ship->peng();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                } else if( message.data()[0] == 7+(id()-1) * 8 ) { // NOLINT
                    m_ship->set_orbit_velocity();
                } else if( message.data()[0] == 8+(id()-1) * 8 ) { // NOLINT
                    set_cloak(!cloak());
                } else if( message.data()[0] == 9+(id()-1) * 8 ) { // NOLINT
                    m_ship->mine();
                    cloak_shot = true;
                    cloak_shot_timer = disable_cloak_time_per_shot;
                }
            }
        }
};

namespace EnDeCoding {
    std::string encodeGameStatus(player_t& player1, player_t& player2) noexcept {
        std::string result = "Game Status: fragments {";
        for(const fragment_ref_t& frag : fragments) {
            result.append(to_string(" %s;", frag->toString().c_str()));
        }
        result.append("} "+player1.toString()+player2.toString()+" Pengs {");
        for(const peng_t& peng : pengs) {
            result.append(peng.toString());
        }
        result.append("} ");
        result.append((event.paused() ? "PAUSE | FINISH " : "ANIMATING | FINISH "));
        return result;
    }
    
    FloatBin encodeGameStatus2(player_t& player1, player_t& player2) noexcept {
        FloatBin result;
        result.put(float(fragments.size()));
        result.put(float(pengs.size()));
        result.put(event.paused());
        for(const fragment_ref_t& frag : fragments) {
            frag->toBin(result);
        }
        player1.toBin(result);
        player2.toBin(result);
        for(const peng_t& peng : pengs) {
            peng.toBin(result);
        }
        return result;
    }
    
    bool decodeGameStatus(std::string code, player_t& player1, player_t& player2) noexcept {
        if(code[0] != 'G') {
            return false;
        }
        char * pos = code.data();
        char * end = code.data() + code.length();
        fragments.clear();
        while(*pos != '{') { ++pos; }
        ++pos;
        while(*pos != '}') {
            fragment_t f({0, 0}, {0, 0}, 0, false);
            if(!f.fromString(pos, end, pos)) {
                std::cout << "fragment";
                return false;
            }
            fragments.push_back(std::make_shared<fragment_t>(f));
        }
        if (!player1.fromString(pos, end, pos)) {
            std::cout << "player1\n";
            //exit(0);
            return false;
        }
        if (!player2.fromString(pos, end, pos)) {
            std::cout << "player2\n";
            exit(0);
            return false;
        }
        pengs.clear();
        while(*pos != '{') { ++pos; }
        ++pos;
        while(*pos != '}') {
            peng_t peng(nullptr, pixel::f2::point_t(0, 0), 0, pixel::f2::vec_t(0, 0), 0);
            if( !peng.fromString(pos, end, pos) ) {
                std::cout << "peng\n";
                exit(0);
                return false;
            }
            pengs.push_back(peng);
        }
        event.set_paused(*(pos+2) == 'P');
        return true;
    }

    bool decodeGameStatus(FloatBin code, player_t& player1, player_t& player2) noexcept {
        const size_t fragmentsSize = size_t(code.get());
        const size_t pengsSize = size_t(code.get());
        event.set_paused(code.get() > 0);
        for(size_t i = 0; i < fragmentsSize; ++i) {
            fragment_t frag({0, 0}, {0, 0}, 0, false);
            if(!frag.fromBin(code)) {
                return false;
            }
            fragments.push_back(std::make_shared<fragment_t>(frag));
        }
        if(player1.fromBin(code)) {
            return false;
        }
        if(player2.fromBin(code)) {
            return false;
        }
        for(size_t i = 0; i < pengsSize; ++i) {
            peng_t peng(nullptr, pixel::f2::point_t(0, 0), 0, pixel::f2::vec_t(0, 0), 0);
            if(peng.fromBin(code)) {
                return false;
            }
            pengs.push_back(peng);
        }
        return true;
    }
    
    bool testAll(player_t& p1, player_t& p2) {
        FloatBin s = encodeGameStatus2(p1, p2);
        std::string e0 = encodeGameStatus(p1, p2);
        decodeGameStatus(s, p1, p2);
        printf("-first %s\n-second %s\n", e0.c_str(), encodeGameStatus(p1, p2).c_str());
        return e0 == encodeGameStatus(p1, p2);
    }
    bool testPlayer(const player_t& p0) {
        FloatBin e0;
        p0.toBin(e0);
        //char* end = nullptr;
        player_t p1(0);
        bool ok_decode = p1.fromBin(e0);
        bool ok = p1 == p0;
        FloatBin e1;
        p1.toBin(e1);
        bool ok_data = e0.data() == e1.data();
        bool ok_eos = e0.readPos() == e0.size();
        printf("Test decode %d, data %d, eos %d (exp %zu, has %zu), origin player == copied player %d, checkOK: %s, checkOK with ship: %s\n", 
            ok_decode, ok_data, ok_eos, e0.size(), e0.readPos(), ok, p0.checkEqual(p1).c_str(), p0.checkShipEqual(p1).c_str());
        printf("- encoded p0: %s\n", p0.toString().c_str());
        printf("- encoded p1: %s\n", p1.toString().c_str());
        if(!ok_data) {
            printf("e0: ");
            for(const float& f : e0.data()) {
                printf("%.10f, ", f);
            }
            printf("\ne1: ");
            for(const float& f : e1.data()) {
                printf("%f, ", f);
            }
            printf("\n\n");
        }
        return ok_decode && ok_data && ok_eos && ok;
    }
    void MB100PerfTest() {
        size_t bytes = 0;
        player_t p1(1);
        int i = 0;
        const uint64_t t0 = getCurrentMilliseconds();
        while(bytes < 100'000'000) {
            std::string s = p1.toString();
            bytes += sizeof(char) * s.length();
            char * endPos = nullptr;
            p1.fromString(s.data(), s.data() + s.length(), endPos);
            ++i;
        }
        const uint64_t td = getCurrentMilliseconds() - t0;        
        printf("conv %.2f MB, dur %fs, throughput %.2f MB/s, %d iter, %.2f bytes/iter\n",
            double(bytes) / 1'000'000.0, 
            double(td) / 1000.0, 
            (double(bytes) / (double(td) / 1000.0)) / 1'000'000.0, 
            i, double(bytes) / (double)i);
    }
    void MB100PerfTest2() {
        size_t bytes = 0;
        player_t p1(1);
        int i = 0;
        FloatBin bb;
        const uint64_t t0 = getCurrentMilliseconds();
        while(bytes < 100'000'000) {
            p1.toBin(bb);
            bytes += sizeof(float) * bb.size();
            p1.fromBin(bb);
            bb.clear();
            ++i;
        }
        const uint64_t td = getCurrentMilliseconds() - t0;        
        printf("conv %.2f MB, dur %fs, throughput %.2f MB/s, %d iter, %.2f bytes/iter\n",
            double(bytes) / 1'000'000.0, 
            double(td) / 1000.0, 
            (double(bytes) / (double(td) / 1000.0)) / 1'000'000.0, 
            i, double(bytes) / (double)i);
    }
};

static int player_count = 2;
static int asteroid_count = 6;
static pixel::f2::point_t tl_text;
static std::string record_bmpseq_basename;
static bool raster = false;
extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_showvelo(bool v) noexcept { show_ship_velo = v; }
}

void print(const std::string& s) {
    printf("[%lu] %s: %s", getElapsedMillisecond(), global_is_server ? "server" : "client", s.c_str());
}

void mainloop() {
    static player_t p1(player_id_1);
    static player_t p2(player_id_2);
    static uint64_t frame_count_total = 0;
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static const int text_height = 35;
    static bool animating = true;
    static float grip = 50;
    int sessionSocket = global_is_server ? p1.sessionSocket() : p2.sessionSocket();
    bool connected = 0<=sessionSocket;

    const uint64_t t1 = animating ? getElapsedMillisecond() : t_last; // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f; // [s]
    t_last = t1;
    
    print("line: 2351\n");
    FloatBin message;
    message.put(100);
    bool key = true;
    if (connected) {
        float typ = 0;
        io::ioEnv->getNetLine(float(t_last), sessionSocket, message, typ);
        key = typ == 0;
    }
    print("line: 2360a\n"); 
    if(event.paused() && animating) {
        FloatBin m;
        m.put(0);
        io::ioEnv->sendNetLine(sessionSocket, m);
    }
    print("line: 2360\n");
    while( pixel::handle_one_event(event) ) {
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            printf("sendedbyte %zu, time[s] %f, sendedbyte / time = %f\n", sendedbyte, double(getElapsedMillisecond()) / 1000, float(sendedbyte) / (float(getElapsedMillisecond()) / 1000));
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);
        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }

        if( event.released_and_clr(pixel::input_event_type_t::RESET)) {
            pengs.clear();
            reset_asteroids(asteroid_count);
            p1.reset(true);
            if(1 < player_count) {
                p2.reset(true);
            }
            FloatBin m;
            m.put(0);
            io::ioEnv->sendNetLine(sessionSocket, m);
        }

        // Pass events to all animated objects
    }
    print("line: 2396\n");
    if(key && message.data()[0] == 0){
        pengs.clear();
        reset_asteroids(asteroid_count);
        p1.reset(true);
        if(1 < player_count) {
            p2.reset(true);
        }
    }
    if(key && message.data()[0] == 1){
        event.set_paused(!event.paused());
    }
    if(!connected){
        if(global_is_server){
            if(p1.serviceGame(global_port)) {
                sessionSocket = p1.sessionSocket();
                connected = true;
            }
        } else {
            if (p2.connectToServer(global_server_name, global_port)) {
                pengs.clear();
                reset_asteroids(asteroid_count);
                p1.reset(true);
                if(1 < player_count) {
                    p2.reset(true);
                }
                FloatBin m;
                m.put(0);
                io::ioEnv->sendNetLine(sessionSocket, m);
                sessionSocket = p2.sessionSocket();
                connected = true;
            }
        }
    }
    if(connected) {
        if(global_is_server) {
            if(990 <= getElapsedMillisecond() % 1000 || getElapsedMillisecond() % 1000 <= 10) {
                std::cout << getElapsedMillisecond() << "\n";
                io::ioEnv->sendNetLine(sessionSocket, EnDeCoding::encodeGameStatus2(p1, p2));
                std::cout << "Server " << EnDeCoding::encodeGameStatus(p1, p2) << std::endl;
            }
        } else if(!key && EnDeCoding::decodeGameStatus(message, p1, p2)) {
            std::cout << "Client " << EnDeCoding::encodeGameStatus(p1, p2) << std::endl;
            //exit(0);
        }
    }
    print("line: 2443\n");
    if(animating) {
        if(global_is_server){
            p1.handle_event02();
            p1.handle_event12(dt);
            if(key) {
                p2.receive_handle_event0(message);
                p2.receive_handle_event1(dt, message);
            }
            p2.tick(dt);
            player_t::collision(p1, p2);
            p1.tick(dt);
        } else {
            p2.handle_event02();
            p2.handle_event12(dt);
            if(key) {
                p1.receive_handle_event0(message);
                p1.receive_handle_event1(dt, message);
            }
            p2.tick(dt);
            player_t::collision(p1, p2);
            p1.tick(dt);
        }
        if (raster) {
            pixel::draw_grid(50, 255, 0, 0, 0, 255, 0, 0, 0);
        }

        // fragments tick
        {
            std::vector<fragment_ref_t> new_fragments;
            for(auto it=fragments.begin(); it != fragments.end(); ) {
                fragment_ref_t f = *it;
                if( !f->tick(dt) ) {
                    it = fragments.erase( it );
                } else if( sun->hit( f->p_center ) ) {
                    it = fragments.erase( it );
                    make_fragments(new_fragments, f, f->velocity.length() * 0.75f, f->rotation_velocity*2.0f);
                } else {
                    ++it;
                }
            }
            fragments.insert(fragments.end(), new_fragments.begin(), new_fragments.end());
        }
        // pengs tick
        {
            for(auto it = pengs.begin(); it != pengs.end(); ) {
                peng_t& p = *it;
                if( p.tick(dt) ) {
                    bool erased = false;
                    for(const peng_t& peng : pengs) {
                        if(p.intersection(peng)) {
                            it = pengs.erase(it);
                            erased = true;
                            break;
                        }
                    }
                    if(!erased) {
                        ++it;      
                    }
                } else {
                    it = pengs.erase(it);
                }
            }
        }
        if( 0 == fragments.size() ) {
            reset_asteroids(asteroid_count);
        }
        sun->tick(dt);
    }
    pixel::clear_pixel_fb(0, 0, 0, 255);

    if(draw_grip){
        pixel::draw_grid(grip, 255, 0, 0, 255, 255, 255, 255, 255);
    }
    // Draw all objects
    pixel::set_pixel_color(rgba_red);
    p1.draw();
    p2.draw();
    pixel::set_pixel_color(rgba_white);
    for(fragment_ref_t &a : fragments) {
        a->draw();
    }
    for(auto & peng : pengs) {
        peng.draw();
    }
    pixel::set_pixel_color(rgba_yellow);
    sun->draw();

    float fps = pixel::gpu_avg_fps();
    tl_text.set(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
    pixel::texture_ref hud_text;
    {
        std::string myShipData;
        {
            player_t myPlayer = global_is_server ? p1 : p2;
            if(!myPlayer.has_ship()) {
                myShipData = "Ship is destroyed!";
            } else {
                std::string c;
                if( cloak_enabled ) {
                    c = to_string(", position %6.2f / %6.2f", p1.center().x, p1.center().y);
                }
                myShipData = to_string("Ship %4d (%4d pengs, %2d mines, %.1f s shield, %4.2f m/s%s)",
                    myPlayer.score(), myPlayer.peng_inventory(), myPlayer.mine_inventory(), myPlayer.shield_time(), myPlayer.velocity(), c.c_str());
            }
        }
        hud_text = pixel::make_text(tl_text, 0, vec4_text_color, text_height, "%s s, fps %4.2f, %s",
                        to_decstring(t1/1000, ',', 5).c_str(), // 1d limit
                        fps, myShipData.c_str());
    }
    pixel::swap_pixel_fb(false);
    {
        const int dx = ( pixel::fb_width - round_to_int((float)hud_text->width*hud_text->dest_sx) ) / 2;
        hud_text->draw_fbcoord(dx, 0);
    }
    pixel::swap_gpu_buffer();
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    ++frame_count_total;
}

static void test_server(const char* exe_path)
{
    int window_width = 800, window_height = 600; // 16:9
    bool enable_vsync = true;
    int sun_gravity_scale_env = 200;    //  20 x 280 =  5600
    int sun_gravity_scale_ships = 200; // 200 x 280 = 56000
    bool use_subsys_primitives = true;
    std::string userRecFname = "bs_user_server_rec.txt";    
    std::string netRecFname = "bs_net_server_rec.txt";

    asteroid_count = 0;
    global_is_server = true;
        
    io::ioEnv = std::make_unique<io::IOEnv>(userRecFname, netRecFname);
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        std::string title = "spacewars (";
        if(global_is_server) title.append("server");
        else title.append("client");
        title.append(")");
        if( !pixel::init_gfx_subsystem(exe_path, title.c_str(), 
            window_width, window_height, origin_norm, enable_vsync, use_subsys_primitives) ) {
            return;
        }
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);

    sun = std::make_shared<star_t>(pixel::f2::point_t(0, 0), spaceship_height,
                                   sun_gravity * (float)sun_gravity_scale_env,
                                   sun_gravity * (float)sun_gravity_scale_ships);
    reset_asteroids(asteroid_count);
    while( true ) { mainloop(); }
}

static void test_client(const char* exe_path)
{
    int window_width = 800, window_height = 600; // 16:9
    bool enable_vsync = true;
    int sun_gravity_scale_env = 200;    //  20 x 280 =  5600
    int sun_gravity_scale_ships = 200; // 200 x 280 = 56000
    bool use_subsys_primitives = true;
    std::string userRecFname = "bs_user_client_rec.txt";    
    std::string netRecFname = "bs_net_client_rec.txt";

    asteroid_count = 0;
    global_is_server = false;
    global_server_name = "localhost";
        
    io::ioEnv = std::make_unique<io::IOEnv>(userRecFname, netRecFname);
    {
        const float origin_norm[] = { 0.5f, 0.5f };
        std::string title = "spacewars (";
        if(global_is_server) title.append("server");
        else title.append("client");
        title.append(")");
        if( !pixel::init_gfx_subsystem(exe_path, title.c_str(), 
            window_width, window_height, origin_norm, enable_vsync, use_subsys_primitives) ) {
            return;
        }
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);

    sun = std::make_shared<star_t>(pixel::f2::point_t(0, 0), spaceship_height,
                                   sun_gravity * (float)sun_gravity_scale_env,
                                   sun_gravity * (float)sun_gravity_scale_ships);
    reset_asteroids(asteroid_count);
    while( true ) { mainloop(); }
}

static bool test_data0() {
    player_t p1(player_id_1);
    player_t p2(player_id_2);
    return EnDeCoding::testPlayer(p1) && EnDeCoding::testAll(p1, p2);
}

static bool test_net_player_client(const player_t &expP, const FloatBin &expB) {
    printf("START: test_net_player_client - encoded expB: %zu\n", (size_t)::pthread_self());
    player_t hasP(player_id_1);
    global_server_name = "sax";
    hasP.connectToServer(global_server_name, global_port);
    FloatBin hasB;
    float typ;
    do {
        io::ioEnv->getNetLine(1, hasP.sessionSocket(), hasB, typ);
    } while(hasB.size() == 0);
    io::ioEnv->sendNetLine(hasP.sessionSocket(), "bla");
    printf("[%zu] receive line", getElapsedMillisecond());
    std::cout << "\ne1: ";
    for(const float & f : hasB.data()) {
        printf("%.10f, ", f);
    }
    if(!hasP.fromBin(hasB)) {
        exit(0);
    }
    bool ok = expB.data() == hasB.data() && hasP == expP;
    std::cout << "\ne0: ";
    for(const float & f : expB.data()) {
        printf("%.10f, ", f);
    }
    std::cout << "\ne1: ";
    for(const float & f : hasB.data()) {
        printf("%.10f, ", f);
    }
    printf("\n- ok %d\n", ok);
    printf("- encoded expB: %s\n", expP.toString().c_str());
    printf("- encoded hasB: %s\n", hasP.toString().c_str());
    printf("END: test_net_player_client - encoded expB: %zu\n", (size_t)::pthread_self());
    return ok;
}
static void test_net_player_server(player_t& p0) {
    printf("START: test_net_player_server - encoded expB: %zu\n", (size_t)::pthread_self());
    FloatBin source;
    p0.serviceGame(global_port);
    printf("[%zu] send line", getElapsedMillisecond());
    std::string s;
    io::ioEnv->sendNetLine(p0.sessionSocket(), p0.toBin(source));
    while(s != "bla"){
        io::ioEnv->getNetLine(1, p0.sessionSocket(), s);
        io::ioEnv->sendNetLine(p0.sessionSocket(), p0.toBin(source));
        printf("[%zu] send line", getElapsedMillisecond());
    }
    printf("END: test_net_player_server - encoded expB: %zu\n", (size_t)::pthread_self());
}
#if 0
static void test_net_world_server(player_t& p0, player_t& p1) {
    // Parent process
    // Parent process: Wait for child to finish
    p0.serviceGame(global_port);
    io::ioEnv->sendNetLine(p0.sessionSocket(), EnDeCoding::encodeGameStatus2(p0, p1));
}
#endif
static bool test_net_player() {
    player_t expP(player_id_1);
    FloatBin expB;
    expP.toBin(expB);
    
    std::thread server(&test_net_player_server, std::ref(expP)); // @suppress("Invalid arguments")
    
    bool res = test_net_player_client(expP, expB);
    
    if (server.joinable()) {
        server.join();
    }
    return res;
}

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1080; // 16:9
    bool enable_vsync = true;
    int sun_gravity_scale_env =   200; // 200 x 280 = 56000
    int sun_gravity_scale_ships = 200; // 200 x 280 = 56000
    bool use_subsys_primitives = true;
    bool test_client_server = false;
    bool test_datastream = false;
    bool test_perf = false;
    bool test_perf2 = false;
    bool test_data_net = false;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    player_count = 2;
    {
        std::string userInFname;
        std::string netInFname;
        std::string userRecFname;
        std::string netRecFname;
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-port", argv[i]) && i+1<argc) {
                asteroid_count = 0;
                global_port = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-to", argv[i]) && i+1<argc) {
                asteroid_count = 0;
                global_is_server = false;
                global_server_name = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-userrec", argv[i]) && i+1<argc) {
                userRecFname = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-netrec", argv[i]) && i+1<argc) {
                netRecFname = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-userin", argv[i]) && i+1<argc) {
                userInFname = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-netin", argv[i]) && i+1<argc) {
                netInFname = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-1p", argv[i]) ) {
                player_count = 1;
            } else if( 0 == strcmp("-3p", argv[i]) ) {
                player_count = 3;
            } else if( 0 == strcmp("-width", argv[i]) && i+1<argc) {
                window_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-height", argv[i]) && i+1<argc) {
                window_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                pixel::set_gpu_forced_fps(atoi(argv[i+1]));
                ++i;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            } else if( 0 == strcmp("-debug_gfx", argv[i]) ) {
                debug_gfx = true;
                show_ship_velo = true;
            } else if( 0 == strcmp("-show_velo", argv[i]) ) {
                show_ship_velo = true;
            } else if( 0 == strcmp("-asteroids", argv[i]) && i+1<argc) {
                asteroid_count = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-sung_env", argv[i]) && i+1<argc) {
                sun_gravity_scale_env = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-sung_ships", argv[i]) && i+1<argc) {
                sun_gravity_scale_ships = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-without_cloak", argv[i]) ) {
                cloak_enabled = false;
            } else if( 0 == strcmp("-soft_prim", argv[i]) ) {
                use_subsys_primitives = false;
            } else if( 0 == strcmp("-draw_grip", argv[i]) ) {
                draw_grip = true;
            } else if( 0 == strcmp("-testcs", argv[i]) ) {
                test_client_server = true;
            } else if( 0 == strcmp("-testdata", argv[i]) ) {
                test_datastream = true;
            } else if( 0 == strcmp("-testperf", argv[i]) ) {
                test_perf = true;
            } else if( 0 == strcmp("-testperf2", argv[i]) ) {
                test_perf2 = true;
            } else if( 0 == strcmp("-testnet", argv[i]) ) {
                test_data_net = true;
            }
        }
        sun = std::make_shared<star_t>(pixel::f2::point_t(0, 0), spaceship_height,
                                       sun_gravity * (float)sun_gravity_scale_env,
                                       sun_gravity * (float)sun_gravity_scale_ships);
        if (userRecFname.empty()) {
            userRecFname = "bs_user_";
            if (global_is_server) {
                userRecFname.append("server_");
            } else {
                userRecFname.append("client_");
            }
            userRecFname.append("rec.txt");
        }
        if (netRecFname.empty()) {
            netRecFname = "bs_net_";
            if (global_is_server) {
                netRecFname.append("server_");
            } else {
                netRecFname.append("client_");
            }
            netRecFname.append("rec.txt");
        }
        io::ioEnv = std::make_unique<io::IOEnv>(userRecFname, netRecFname);
        if( !userInFname.empty() ) {
            io::ioEnv->setUserIn(userInFname);
        }
        if( !netInFname.empty() ) {
            io::ioEnv->setNetIn(netInFname);
        }
        if (test_client_server) {
            pid_t child_pid = ::fork();

            if( child_pid == -1 ) {
                // Handle fork failure
                std::cerr << "fork() failed: " << strerror(errno) << std::endl;
                return 1;
            }
            if( child_pid == 0 ) {
                // Child process                
                test_client(argv[0]);
                exit(EXIT_SUCCESS);
            } else {
                // Parent process
                test_server(argv[0]);
                // Parent process: Wait for child to finish
                int child_status;
                // Wait for the specific child (child_pid) to exit
                if( ::waitpid(child_pid, &child_status, 0) == -1 ) {
                    std::cerr << "waitpid() failed: " << strerror(errno) << std::endl;
                    return 1;
                }
                // Check if child exited successfully
                if( WIFEXITED(child_status) ) {
                    std::cout << "Child exited with status: " << WEXITSTATUS(child_status) << std::endl;
                } else {
                    std::cerr << "Child terminated abnormally" << std::endl;
                }
            }
            return 0;
        } else if(test_datastream) {
            if(test_data0()) {
                printf("Test is sucsessful!\n");
            }
            return 0;            
        } else if(test_perf) {
            EnDeCoding::MB100PerfTest();
            return 0;
        } else if(test_perf2) {
            EnDeCoding::MB100PerfTest2();
            return 0;
        } else if(test_data_net) {
            test_net_player();
            return 0;
        }
    }
    {
        const uint64_t elapsed_ms = getElapsedMillisecond();
        log_printf(elapsed_ms, "Usage %s -1p -width <int> -height <int> -record <bmp-files-basename> -fps <int> -no_vsync"
                                      " -debug_gfx -show_velo -asteroids <int> -sung_env <int> -sung_ships <int>\n", argv[0]);
        log_printf(elapsed_ms, "- win size %d x %d\n", window_width, window_height);
        log_printf(elapsed_ms, "- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
        log_printf(elapsed_ms, "- subsys_primitives %d\n", use_subsys_primitives);
        log_printf(elapsed_ms, "- enable_vsync %d\n", enable_vsync);
        log_printf(elapsed_ms, "- forced_fps %d\n", pixel::gpu_forced_fps());
        log_printf(elapsed_ms, "- debug_gfx %d\n", debug_gfx);
        log_printf(elapsed_ms, "- show_ship_velo %d\n", show_ship_velo);
        log_printf(elapsed_ms, "- players %d\n", player_count);
        log_printf(elapsed_ms, "- raster %d\n", raster);
        log_printf(elapsed_ms, "- asteroid_count %d\n", asteroid_count);
        log_printf(elapsed_ms, "- sun_gravity_scale_env %d -> %f [m/s^2]\n", sun_gravity_scale_env, sun_gravity * (float)sun_gravity_scale_env);
        log_printf(elapsed_ms, "- sun_gravity_scale_ships %d -> %f [m/s^2]\n", sun_gravity_scale_ships, sun_gravity * (float)sun_gravity_scale_ships);
        log_printf(elapsed_ms, "- cloak enabled %d\n", cloak_enabled);
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        std::string title = "spacewars (";
        if(global_is_server) title.append("server");
        else title.append("client");
        title.append(")");
        if( !pixel::init_gfx_subsystem(argv[0], title.c_str(), 
            window_width, window_height, origin_norm, enable_vsync, use_subsys_primitives) ) {
            return 1;
        }
    }
    pixel::cart_coord.set_height(-space_height/2.0f, space_height/2.0f);

    /*
    const float peng_diag = spaceship_height;
    for(int i = 0; i < 5; ++i){
        const float x = pixel::next_rnd() * pixel::cart_coord.width();
        const float y = pixel::next_rnd() * pixel::cart_coord.height();
        peng_t peng(&world_id, pixel::f2::point_t(x, y), peng_diag, 0.0f, 0.0f);
        pengs.emplace_back(peng);
        if(!peng.on_screen()){
            printf("peng %d is not on screen", i);
        }
    }
    */
    reset_asteroids(asteroid_count);
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
