// *****************************************************************************
//
// © Copyright 2020, Septentrio NV/SA.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    1. Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//    2. Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//    3. Neither the name of the copyright holder nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// *****************************************************************************

#pragma once

// C++
#include <thread>

// Linux
#include <linux/input.h>
#include <linux/serial.h>

// Boost
#include <boost/asio.hpp>

// pcap
#include <pcap.h>

// ROSaic
#include <septentrio_gnss_driver/abstraction/typedefs.hpp>

//! Possible baudrates for the Rx
const static std::array<uint32_t, 21> baudrates = {
    1200,    2400,    4800,    9600,    19200,   38400,   57600,
    115200,  230400,  460800,  500000,  576000,  921600,  1000000,
    1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000};

namespace io {

    class TcpIo
    {
    public:
        TcpIo(ROSaicNodeBase* node) : node_(node) {}

        ~TcpIo() { stream_->close(); }

        void close() { stream_->close(); }

        [[nodiscard]] bool connect()
        {
            boost::asio::ip::tcp::resolver::iterator endpointIterator;

            try
            {
                boost::asio::ip::tcp::resolver resolver(ioService_);
                boost::asio::ip::tcp::resolver::query query(
                    node_->settings()->tcp_ip, node_->settings()->tcp_port);
                endpointIterator = resolver.resolve(query);
            } catch (std::runtime_error& e)
            {
                node_->log(LogLevel::ERROR,
                           "Could not resolve " + node_->settings()->tcp_ip +
                               " on port " + node_->settings()->tcp_port + ": " +
                               e.what());
                return false;
            }

            stream_.reset(new boost::asio::ip::tcp::socket(ioService_));

            node_->log(LogLevel::INFO, "Connecting to tcp://" +
                                           node_->settings()->tcp_ip + ":" +
                                           node_->settings()->tcp_port + "...");

            try
            {
                stream_->connect(*endpointIterator);

                stream_->set_option(boost::asio::ip::tcp::no_delay(true));

                node_->log(LogLevel::INFO,
                           "Connected to " + endpointIterator->host_name() + ":" +
                               endpointIterator->service_name() + ".");
            } catch (std::runtime_error& e)
            {
                node_->log(LogLevel::ERROR,
                           "Could not connect to " + endpointIterator->host_name() +
                               ": " + endpointIterator->service_name() + ": " +
                               e.what());
                return false;
            }
            return true;
        }

    private:
        ROSaicNodeBase* node_;

    public:
        boost::asio::io_service ioService_;
        std::unique_ptr<boost::asio::ip::tcp::socket> stream_;
    };

    struct SerialIo
    {
        SerialIo(ROSaicNodeBase* node) :
            node_(node), flowcontrol_(node->settings()->hw_flow_control),
            baudrate_(node->settings()->baudrate)
        {
            stream_.reset(new boost::asio::serial_port(ioService_));
        }

        ~SerialIo() { stream_->close(); }

        void close() { stream_->close(); }

        [[nodiscard]] bool connect()
        {
            if (stream_->is_open())
            {
                stream_->close();
            }

            bool opened = false;

            while (!opened)
            {
                try
                {
                    node_->log(LogLevel::INFO,
                               "Connecting serially to device" +
                                   node_->settings()->device +
                                   ", targeted baudrate: " +
                                   std::to_string(node_->settings()->baudrate));
                    stream_->open(port_);
                    opened = true;
                } catch (const boost::system::system_error& err)
                {
                    node_->log(LogLevel::ERROR,
                               "SerialCoket: Could not open serial port " + port_ +
                                   ". Error: " + err.what() +
                                   ". Will retry every second.");

                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(1s);
                }
            }

            // No Parity, 8bits data, 1 stop Bit
            stream_->set_option(boost::asio::serial_port_base::baud_rate(baudrate_));
            stream_->set_option(boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none));
            stream_->set_option(boost::asio::serial_port_base::character_size(8));
            stream_->set_option(boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));
            stream_->set_option(boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none));

            int fd = stream_->native_handle();
            termios tio;
            // Get terminal attribute, follows the syntax
            // int tcgetattr(int fd, struct termios *termios_p);
            tcgetattr(fd, &tio);

            // Hardware flow control settings
            if (flowcontrol_ == "RTS|CTS")
            {
                tio.c_iflag &= ~(IXOFF | IXON);
                tio.c_cflag |= CRTSCTS;
            } else
            {
                tio.c_iflag &= ~(IXOFF | IXON);
                tio.c_cflag &= ~CRTSCTS;
            }
            // Setting serial port to "raw" mode to prevent EOF exit..
            cfmakeraw(&tio);

            // Commit settings, syntax is
            // int tcsetattr(int fd, int optional_actions, const struct termios
            // *termios_p);
            tcsetattr(fd, TCSANOW, &tio);
            // Set low latency
            struct serial_struct serialInfo;

            ioctl(fd, TIOCGSERIAL, &serialInfo);
            serialInfo.flags |= ASYNC_LOW_LATENCY;
            ioctl(fd, TIOCSSERIAL, &serialInfo);

            return setBaudrate();
        }

        [[nodiscard]] bool setBaudrate()
        {
            // Setting the baudrate, incrementally..
            node_->log(LogLevel::DEBUG,
                       "Gradually increasing the baudrate to the desired value...");
            boost::asio::serial_port_base::baud_rate current_baudrate;
            node_->log(LogLevel::DEBUG, "Initiated current_baudrate object...");
            try
            {
                stream_->get_option(current_baudrate); // Note that this sets
                                                       // current_baudrate.value()
                                                       // often to 115200, since by
                                                       // default, all Rx COM ports,
                // at least for mosaic Rxs, are set to a baudrate of 115200 baud,
                // using 8 data-bits, no parity and 1 stop-bit.
            } catch (boost::system::system_error& e)
            {

                node_->log(LogLevel::ERROR,
                           "get_option failed due to " + std::string(e.what()));
                node_->log(LogLevel::INFO, "Additional info about error is " +
                                               boost::diagnostic_information(e));
                /*
                boost::system::error_code e_loop;
                do // Caution: Might cause infinite loop..
                {
                    stream_->get_option(current_baudrate, e_loop);
                } while(e_loop);
                */
                return false;
            }
            // Gradually increase the baudrate to the desired value
            // The desired baudrate can be lower or larger than the
            // current baudrate; the for loop takes care of both scenarios.
            node_->log(LogLevel::DEBUG,
                       "Current baudrate is " +
                           std::to_string(current_baudrate.value()));
            for (uint8_t i = 0; i < baudrates.size(); i++)
            {
                if (current_baudrate.value() == baudrate_)
                {
                    break; // Break if the desired baudrate has been reached.
                }
                if (current_baudrate.value() >= baudrates[i] &&
                    baudrate_ > baudrates[i])
                {
                    continue;
                }
                // Increment until Baudrate[i] matches current_baudrate.
                try
                {
                    stream_->set_option(
                        boost::asio::serial_port_base::baud_rate(baudrates[i]));
                } catch (boost::system::system_error& e)
                {

                    node_->log(LogLevel::ERROR,
                               "set_option failed due to " + std::string(e.what()));
                    node_->log(LogLevel::INFO, "Additional info about error is " +
                                                   boost::diagnostic_information(e));
                    return false;
                }
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(500ms);

                try
                {
                    stream_->get_option(current_baudrate);
                } catch (boost::system::system_error& e)
                {

                    node_->log(LogLevel::ERROR,
                               "get_option failed due to " + std::string(e.what()));
                    node_->log(LogLevel::INFO, "Additional info about error is " +
                                                   boost::diagnostic_information(e));
                    /*
                    boost::system::error_code e_loop;
                    do // Caution: Might cause infinite loop..
                    {
                        stream_->get_option(current_baudrate, e_loop);
                    } while(e_loop);
                    */
                    return false;
                }
                node_->log(LogLevel::DEBUG,
                           "Set ASIO baudrate to " +
                               std::to_string(current_baudrate.value()));
            }
            node_->log(LogLevel::INFO, "Set ASIO baudrate to " +
                                           std::to_string(current_baudrate.value()) +
                                           ", leaving InitializeSerial() method");
            return true;
        }

    private:
        ROSaicNodeBase* node_;
        std::string flowcontrol_;
        std::string port_;
        uint32_t baudrate_;

    public:
        boost::asio::io_service ioService_;
        std::unique_ptr<boost::asio::serial_port> stream_;
    };

    class SbfFileIo
    {
    public:
        SbfFileIo(ROSaicNodeBase* node) : node_(node) {}

        ~SbfFileIo() { stream_->close(); }

        void close() { stream_->close(); }

        [[nodiscard]] bool connect()
        {
            node_->log(LogLevel::INFO, "Opening SBF file stream" +
                                           node_->settings()->device + "...");

            int fd = open(node_->settings()->device.c_str(), O_RDONLY);
            if (fd == -1)
            {
                node_->log(LogLevel::ERROR, "open SBF file failed.");
                return false;
            }
            stream_.reset(new boost::asio::posix::stream_descriptor(ioService_));

            try
            {
                stream_->assign(fd);

            } catch (std::runtime_error& e)
            {
                node_->log(LogLevel::ERROR, "assigning SBF file failed due to " +
                                                std::string(e.what()));
                return false;
            }
            return true;
        }

    private:
        ROSaicNodeBase* node_;

    public:
        boost::asio::io_service ioService_;
        std::unique_ptr<boost::asio::posix::stream_descriptor> stream_;
    };

    class PcapFileIo
    {
    public:
        PcapFileIo(ROSaicNodeBase* node) : node_(node) {}

        ~PcapFileIo()
        {
            pcap_close(pcap_);
            stream_->close();
        }

        void close()
        {
            pcap_close(pcap_);
            stream_->close();
        }

        [[nodiscard]] bool connect()
        {
            node_->log(LogLevel::INFO, "Opening pcap file stream" +
                                           node_->settings()->device + "...");

            stream_.reset(new boost::asio::posix::stream_descriptor(ioService_));

            pcap_ = pcap_open_offline(node_->settings()->device.c_str(),
                                      errBuff_.data());
            stream_->assign(pcap_get_selectable_fd(pcap_));

            try
            {

            } catch (std::runtime_error& e)
            {
                return false;
            }
            return true;
        }

    private:
        ROSaicNodeBase* node_;
        std::array<char, 100> errBuff_;
        pcap_t* pcap_;

    public:
        boost::asio::io_service ioService_;
        std::unique_ptr<boost::asio::posix::stream_descriptor> stream_;
    };
} // namespace io