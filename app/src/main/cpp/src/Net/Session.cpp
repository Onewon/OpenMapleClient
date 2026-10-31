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
#include "Session.h"

#include "../Configuration.h"
#include "PacketError.h"
#include <algorithm>
#include <android/log.h>
#include "../IO/AndroidInput.h"

namespace ms {
Session::Session(std::unique_ptr<Forwarder> packet_forwarder) :
    packet_forwarder_(std::move(packet_forwarder)),
    length_(0),
    pos_(0),
    is_connected_(false) {}

Session::~Session() {
    if (is_connected_) {
        ++connection_generation_;
        socket_.close();
    }
}

bool Session::init(const char *host, const char *port) {
    AndroidInput::connection_lost.store(false);
    // Connect to the server
    is_connected_ = socket_.open(host, port);

    if (is_connected_) {
        // Read keys necessary for communicating with the server
        cryptography_ = { socket_.get_buffer() };
    }

    return is_connected_;
}

std::string Session::readConnectionValue(GLFMDisplay *pApp, const char *filename) {
    std::string filePath = std::string(((ANativeActivity*)glfmGetAndroidActivity(pApp))->externalDataPath) + "/" + filename;
    FILE* file = fopen(filePath.c_str(), "rb");
    std::string hostIP;

    if (file != nullptr) {
        char buffer[256]; // Buffer to store read data
        size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
        fclose(file);

        // Ensure the read operation succeeded
        if (bytesRead > 0) {
            // Convert the buffer to a string
            hostIP.assign(buffer, bytesRead);
        }
    }

    return hostIP;
}

Error Session::init(GLFMDisplay *pApp) {
    std::string HOST = readConnectionValue(pApp, "hostip.txt");
    std::string PORT = readConnectionValue(pApp, "serverport.txt");
    channel_host_ = readConnectionValue(pApp, "channelhost.txt");
    if (PORT.empty()) {
        PORT = "18484";
    }
    Setting<ServerIP>::get().save(HOST);
    Setting<ServerPort>::get().save(PORT);

    if (!init(HOST.c_str(), PORT.c_str())) {
        return Error::CONNECTION;
    }

    return Error::NONE;
}

void Session::reconnect(const char *address, const char *port) {
    ++connection_generation_;
    // Close the current connection and open a new one
    bool success = socket_.close();

    if (success) {
        pos_ = 0;
        length_ = 0;
        header_pos_ = 0;
        if (!init(channel_host_.empty() ? address : channel_host_.c_str(), port)) AndroidInput::connection_lost.store(true);
    } else {
        is_connected_ = false;
        AndroidInput::connection_lost.store(true);
    }
}

void Session::process(const int8_t *bytes, size_t available) {
    while (available > 0 && is_connected_) {
        if (header_pos_ < HEADER_LENGTH) {
            const size_t count = std::min(HEADER_LENGTH - header_pos_, available);
            memcpy(header_ + header_pos_, bytes, count);
            header_pos_ += count;
            bytes += count;
            available -= count;
            if (header_pos_ < HEADER_LENGTH) {
                return;
            }
            length_ = cryptography_.check_length(header_);
            if (length_ < OPCODE_LENGTH || length_ > sizeof(buffer_)) {
                __android_log_print(ANDROID_LOG_ERROR, "Session", "Invalid packet length: %zu", length_);
                is_connected_ = false;
                socket_.close();
                AndroidInput::connection_lost.store(true);
                return;
            }
        }
        const size_t count = std::min(length_ - pos_, available);
        memcpy(buffer_ + pos_, bytes, count);
        pos_ += count;
        bytes += count;
        available -= count;
        if (pos_ == length_) {
            cryptography_.decrypt(buffer_, length_);
            // A handler can reconnect and replace the socket buffer.
            pos_ = 0;
            header_pos_ = 0;
            const size_t packet_length = length_;
            length_ = 0;
            const auto generation = connection_generation_;
            try {
                packet_forwarder_->forward(buffer_, packet_length);
            } catch (const PacketError &err) {
                __android_log_print(ANDROID_LOG_ERROR, "Session", "Packet handler failed: %s", err.what());
            }
            if (generation != connection_generation_) {
                return;
            }
        }
    }
}

void Session::write(int8_t *packet_bytes, size_t packet_length) {
    if (!is_connected_) {
        return;
    }

    int8_t header[HEADER_LENGTH];
    cryptography_.create_header(header, packet_length);
    cryptography_.encrypt(packet_bytes, packet_length);

    if (!socket_.dispatch(header, HEADER_LENGTH) || !socket_.dispatch(packet_bytes, packet_length)) {
        __android_log_print(ANDROID_LOG_ERROR, "Session", "Send failed; connection closed");
        is_connected_ = false;
        socket_.close();
        AndroidInput::connection_lost.store(true);
    }
}

void Session::read() {
    // Check if a packet has arrived. Handle if data is sufficient: 4 bytes
    // (header) + 2 bytes (opcode) = 6 bytes.
    size_t result = socket_.receive(&is_connected_);
    if (!is_connected_) {
        AndroidInput::connection_lost.store(true);
        socket_.close();
        return;
    }

    if (result > 0) {
        // Retrieve buffer from the socket and process it
        const int8_t *bytes = socket_.get_buffer();
        process(bytes, result);
    }
}

void Session::reconnect() {
    std::string HOST = Setting<ServerIP>::get().load();
    std::string PORT = Setting<ServerPort>::get().load();

    reconnect(HOST.c_str(), PORT.c_str());
}

bool Session::is_connected() const {
    return is_connected_;
}
}  // namespace ms
