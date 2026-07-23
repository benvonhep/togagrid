// ══════════════════════════════════════════════════════════════
//  togaws_config.h — the only file you edit per location.
// ══════════════════════════════════════════════════════════════
#pragma once

// This node's name on the network: http://toga.local/ (mDNS), and the OTA
// hostname (./flash-ota.sh webserver toga.local).
#define WS_HOSTNAME "toga"

// The WLAN itself is not configured here any more. The CONTROLLER hosts it —
// SSID/password live in <toga_ota.h> (TOGA_AP_SSID / TOGA_AP_PASS) and the
// joining is done by togaWifiBegin() in <toga_proto.h>, same as every node.
//
// This used to carry a long warning that the router had to be on TOGA_CHANNEL,
// because an ESP32 has one radio and joining a router's WLAN dragged this node
// onto the router's channel — leaving it deaf to the very ESP-NOW bus it exists
// to watch. That is gone: the AP is ours and pinned to TOGA_CHANNEL, so being
// on the network and being on the bus are no longer in conflict. The page still
// shows the channel and warns on a mismatch, but it should never fire now.
