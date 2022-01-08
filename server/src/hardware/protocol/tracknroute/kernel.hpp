/**
 * server/src/hardware/protocol/tracknroute/kernel.hpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2021-2022 Reinder Feenstra
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TRAINTASTIC_SERVER_HARDWARE_PROTOCOL_TRACKNROUTE_KERNEL_HPP
#define TRAINTASTIC_SERVER_HARDWARE_PROTOCOL_TRACKNROUTE_KERNEL_HPP

#include <array>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <traintastic/enum/tristate.hpp>
#include "config.hpp"
#include "iohandler/iohandler.hpp"

class InputController;

namespace TracknRoute {

struct Message;

class Kernel
{
  public:
    static constexpr uint16_t inputAddressMin = 1;
    static constexpr uint16_t inputAddressMax = 16;

  private:
    boost::asio::io_context m_ioContext;
    std::unique_ptr<IOHandler> m_ioHandler;
    std::thread m_thread;
    std::string m_logId;
    boost::asio::steady_timer m_startupDelayTimer;
    std::function<void()> m_onStarted;

    InputController* m_inputController;
    std::array<TriState, inputAddressMax> m_inputValues;

    Config m_config;
#ifndef NDEBUG
    bool m_started;
#endif

    Kernel(const Config& config);

    void setIOHandler(std::unique_ptr<IOHandler> handler);

    template<class T>
    void postSend(const T& message)
    {
      m_ioContext.post(
        [this, message]()
        {
          send(message);
        });
    }

    void send(const Message& message);

    void startupDelayExpired(const boost::system::error_code& ec);

  public:
    Kernel(const Kernel&) = delete;
    Kernel& operator =(const Kernel&) = delete;

    /**
     * @brief IO context for Track 'n Route kernel and IO handler
     * @return The IO context
     */
    boost::asio::io_context& ioContext() { return m_ioContext; }

    /**
     * @brief Create kernel and IO handler
     * @param[in] config Track 'n Route configuration
     * @param[in] args IO handler arguments
     * @return The kernel instance
     */
    template<class IOHandlerType, class... Args>
    static std::unique_ptr<Kernel> create(const Config& config, Args... args)
    {
      static_assert(std::is_base_of_v<IOHandler, IOHandlerType>);
      std::unique_ptr<Kernel> kernel{new Kernel(config)};
      kernel->setIOHandler(std::make_unique<IOHandlerType>(*kernel, std::forward<Args>(args)...));
      return kernel;
    }

    /**
     * @brief Access the IO handler
     * @return The IO handler
     * @note The IO handler runs in the kernel's IO context, not all functions can be called safely!
     */
    template<class T>
    T& ioHandler()
    {
      assert(dynamic_cast<T*>(m_ioHandler.get()));
      return static_cast<T&>(*m_ioHandler);
    }

    /**
     *
     *
     */
    inline const std::string& logId() { return m_logId; }

    /**
     * @brief Set object id used for log messages
     * @param[in] value The object id
     */
    inline void setLogId(std::string value)
    {
      m_logId = std::move(value);
    }

    /**
     * @brief Set Track 'n Route configuration
     * @param[in] config The Track 'n Route configuration
     */
    void setConfig(const Config& config);

    /**
     * @brief ...
     * @param[in] callback ...
     * @note This function may not be called when the kernel is running.
     */
    inline void setOnStarted(std::function<void()> callback)
    {
      assert(!m_started);
      m_onStarted = std::move(callback);
    }

    /**
     * @brief Set the input controller
     *
     * @param[in] inputController The input controller
     * @note This function may not be called when the kernel is running.
     */
    inline void setInputController(InputController* inputController)
    {
      assert(!m_started);
      m_inputController = inputController;
    }

    /**
     * @brief Start the kernel and IO handler
     */
    void start();

    /**
     * @brief Stop the kernel and IO handler
     */
    void stop();

    /**
     * @brief ...
     *
     * This must be called by the IO handler whenever a XpressNet message is received.
     *
     * @param[in] message The received XpressNet message
     * @note This function must run in the kernel's IO context
     */
    void receive(const Message& message);
};

}

#endif
