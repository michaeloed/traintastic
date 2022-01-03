/**
 * server/src/hardware/protocol/dccplusplus/kernel.hpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2021 Reinder Feenstra
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

#ifndef TRAINTASTIC_SERVER_HARDWARE_PROTOCOL_DCCPLUSPLUS_KERNEL_HPP
#define TRAINTASTIC_SERVER_HARDWARE_PROTOCOL_DCCPLUSPLUS_KERNEL_HPP

#include <array>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <traintastic/enum/tristate.hpp>
#include "config.hpp"
#include "iohandler/iohandler.hpp"

class Decoder;
enum class DecoderChangeFlags;
class DecoderController;

namespace DCCPlusPlus {

struct Message;

class Kernel
{
  private:
    boost::asio::io_context m_ioContext;
    std::unique_ptr<IOHandler> m_ioHandler;
    std::thread m_thread;
    std::string m_logId;
    boost::asio::steady_timer m_startupDelayTimer;
    std::function<void()> m_onStarted;

    TriState m_powerOn;
    TriState m_emergencyStop;
    std::function<void(bool)> m_onPowerOnChanged;

    DecoderController* m_decoderController;

    Config m_config;
#ifndef NDEBUG
    bool m_started;
#endif

    Kernel(const Config& config);

    void setIOHandler(std::unique_ptr<IOHandler> handler);

    void postSend(const std::string& message)
    {
      m_ioContext.post(
        [this, message]()
        {
          send(message);
        });
    }

    void send(std::string_view message);

    void startupDelayExpired(const boost::system::error_code& ec);

  public:
    Kernel(const Kernel&) = delete;
    Kernel& operator =(const Kernel&) = delete;

    /**
     * @brief IO context for DCC++ kernel and IO handler
     *
     * @return The IO context
     */
    boost::asio::io_context& ioContext() { return m_ioContext; }

    /**
     * @brief Create kernel and IO handler
     *
     * @param[in] config DCC++ configuration
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
     *
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
     *
     * @param[in] value The object id
     */
    inline void setLogId(std::string value)
    {
      m_logId = std::move(value);
    }

    /**
     * @brief Set DCC++ configuration
     *
     * @param[in] config The DCC++ configuration
     */
    void setConfig(const Config& config);

    /**
     * @brief ...
     *
     * @param[in] callback ...
     * @note This function may not be called when the kernel is running.
     */
    inline void setOnStarted(std::function<void()> callback)
    {
      assert(!m_started);
      m_onStarted = std::move(callback);
    }

    /**
     * @brief ...
     *
     * @param[in] callback ...
     * @note This function may not be called when the kernel is running.
     */
    inline void setOnPowerOnChanged(std::function<void(bool)> callback)
    {
      assert(!m_started);
      m_onPowerOnChanged = std::move(callback);
    }

    /**
     * @brief Set the decoder controller
     *
     * @param[in] decoderController The decoder controller
     * @note This function may not be called when the kernel is running.
     */
    inline void setDecoderController(DecoderController* decoderController)
    {
      assert(!m_started);
      m_decoderController = decoderController;
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
     * This must be called by the IO handler whenever a DCC++ message is received.
     *
     * @param[in] message The received DCC++ message
     * @note This function must run in the kernel's IO context
     */
    void receive(std::string_view message);

    /**
     *
     *
     */
    void powerOn();

    /**
     *
     *
     */
    void powerOff();

    /**
     *
     *
     */
    void emergencyStop();

    /**
     *
     *
     */
    void clearEmergencyStop();

    /**
     *
     *
     */
    void decoderChanged(const Decoder& decoder, DecoderChangeFlags changes, uint32_t functionNumber);
};

}

#endif