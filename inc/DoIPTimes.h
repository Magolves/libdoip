#ifndef DOIPTIMES_H
#define DOIPTIMES_H

#include <stdint.h>

// Table 12
namespace doip::times {

/**
 * @brief Client-specific times.
 */
namespace client {
/**
 * @brief Minimum time in ms to wait for a response after an UDP message was sent.
 * @ note tA_DoIP_Ctrl
 */
constexpr uint32_t UdpMessageTimeout = 2000;

/**
 * @brief Maximum time in ms to wait for
 * @note tA_DoIP_Routing_Activation
 */
constexpr uint32_t RoutingActivationResponseTimeout = 2000;

/**
 * @brief Time to wait before sending the next message after a client sent a message which requires no response.
 * @note tA_Processing_Time
 */
constexpr uint32_t IntermessageDelay = 2000;

} // namespace client

/**
 * @brief Server-specific times.
 */
namespace server {
/**
 * @brief Max. time in ms to wait before a server responds to
 * a vehicle announcement request. The server should use a *random number*
 * between 0 and `VehicleAnnouncementDelay`.
 * @note tA_DoIP_Announce_Wait
 */
constexpr uint32_t VehicleAnnouncementDelay = 500;

/**
 * @brief Interval between vehicle announcements in ms.
 * @note tA_DoIP_Announce_Interval
 */
constexpr uint32_t VehicleAnnouncementInterval = 500;

/**
 * @brief Number of vehicle announcements to be send.
 * @note tA_DoIP_Announce_Num
 */
constexpr uint8_t VehicleAnnouncementNumber = 3;

/**
 * @brief Maximum inactivity time in ms after a TCP connection was accepted. If no
 * routing activation request is received within this period, the socket shall be closed
 * @note tT_TCP_Initial_Inactivity
 */
constexpr uint32_t InitialInactivityTimeout = 2000;

/**
 * @brief Maximum inactivity time in ms after a TCP message was sent or received.
 * @note tT_TCP_General_Inactivity
 */
constexpr uint32_t GeneralInactivityTimeout = 300000;

/**
 * @brief Time in ms to wait for a alive check response.
 * @note tT_TCP_Alive_Check
 */
constexpr uint32_t AliveCheckResponseTimeout = 500;

/**
 * @brief Time in ms to wait for sync after server has sent a vehicle identification response
 * with sync status 'incomplete' (0x10).
 * @note tT_TCP_Alive_Check
 */
constexpr uint32_t VehicleDiscoveryTimer = 500;
} // namespace server


/**
 * @brief Maximum time in ms to wait for a response (ACK/NACK) after a diagnostic message
 * has been sent.
 * @note tA_DoIP_Diagnostic_Message
 */
constexpr uint32_t DiagnosticMessageResponseTimeout = 2000;

} // namespace doip::times

#endif /* DOIPTIMES_H */
