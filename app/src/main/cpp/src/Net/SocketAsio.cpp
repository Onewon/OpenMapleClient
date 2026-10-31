//	This file is part of the continued Journey MMORPG client
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Affero General Public License for more details.
//
//	You should have received a copy of the GNU Affero General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include <android/log.h>
#include <poll.h>
#include "SocketAsio.h"

#ifdef USE_ASIO
namespace ms {
SocketAsio::SocketAsio() : resolver_(ioservice_), socket_(ioservice_) {}

SocketAsio::~SocketAsio() {
    if (socket_.is_open()) {
        error_code error;
        socket_.close(error);
    }
}

bool SocketAsio::open(const char *address, const char *port) {
    error_code error;
    tcp::resolver::query query(address, port);
    tcp::resolver::iterator endpointiter = resolver_.resolve(query, error);
    if (!error) {
        asio::connect(socket_, endpointiter, error);
    }

    if (!error) {
        size_t result = asio::read(socket_, asio::buffer(buffer_, HANDSHAKE_LEN), error);
        if (!error && result == HANDSHAKE_LEN) {
#ifdef USE_CRYPTO
            const unsigned version = static_cast<uint8_t>(buffer_[2])
                | (static_cast<uint8_t>(buffer_[3]) << 8);
            if (buffer_[0] != 14 || buffer_[1] != 0 || version != 83 || buffer_[15] != 8) {
                __android_log_print(ANDROID_LOG_ERROR, "SocketAsio", "Unsupported handshake from %s:%s", address, port);
                error_code ignored;
                socket_.close(ignored);
                return false;
            }
            __android_log_print(ANDROID_LOG_INFO, "SocketAsio", "Connected %s:%s GMS v%u locale %u", address, port, version, static_cast<uint8_t>(buffer_[15]));
#endif
            return true;
        }
    }

    __android_log_print(ANDROID_LOG_ERROR, "SocketAsio", "Connection %s:%s failed: %s", address, port, error.message().c_str());
    error_code ignored;
    socket_.close(ignored);
    return false;
}

bool SocketAsio::close() {
    if (!socket_.is_open()) return true;
    error_code error;
    socket_.shutdown(tcp::socket::shutdown_both, error);
    socket_.close(error);

    return !error;
}

size_t SocketAsio::receive(bool *recvok) {
    if (!socket_.is_open()) { *recvok = false; return 0; }
    pollfd descriptor{socket_.native_handle(), POLLIN, 0};
    const int ready = poll(&descriptor, 1, 0);
    if (ready > 0) {
        error_code error;
        size_t result = socket_.read_some(asio::buffer(buffer_), error);
        *recvok = !error;
        if (error) __android_log_print(ANDROID_LOG_ERROR, "SocketAsio", "Receive disconnected: %s", error.message().c_str());

        return result;
    }

    return 0;
}

const int8_t *SocketAsio::get_buffer() const {
    return buffer_;
}

bool SocketAsio::dispatch(const int8_t *bytes, size_t length) {
    error_code error;
    size_t result = asio::write(socket_, asio::buffer(bytes, length), error);

    return !error && (result == length);
}
}  // namespace ms
#endif
