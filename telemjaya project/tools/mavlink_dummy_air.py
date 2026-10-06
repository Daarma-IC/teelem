#!/usr/bin/env python3
"""Inject a small ArduPilot-like MAVLink v1 stream into TelemJaya Air USB.

This deliberately has no pymavlink dependency.  It only needs pyserial.
The Air firmware changes its USB UART to 57600 baud after boot, so this tool
can use the onboard CH9102 as a temporary dummy flight controller.
"""

import argparse
import math
import struct
import sys
import time

import serial


MAV_TYPE_QUADROTOR = 2
MAV_AUTOPILOT_ARDUPILOTMEGA = 3
MAV_MODE_FLAG_CUSTOM_MODE_ENABLED = 1
MAV_MODE_FLAG_STABILIZE_ENABLED = 16
MAV_MODE_FLAG_MANUAL_INPUT_ENABLED = 64
MAV_STATE_STANDBY = 3


def x25_accumulate(byte: int, crc: int) -> int:
    tmp = byte ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    return (
        ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    )


def mavlink_v1(seq: int, msgid: int, payload: bytes, crc_extra: int,
               sysid: int = 1, compid: int = 1) -> bytes:
    header = bytes((len(payload), seq & 0xFF, sysid, compid, msgid))
    crc = 0xFFFF
    for byte in header + payload:
        crc = x25_accumulate(byte, crc)
    crc = x25_accumulate(crc_extra, crc)
    return b"\xFE" + header + payload + struct.pack("<H", crc)


class DummyVehicle:
    def __init__(self) -> None:
        self.seq = 0
        self.started = time.monotonic()

    def packet(self, msgid: int, payload: bytes, crc_extra: int) -> bytes:
        frame = mavlink_v1(self.seq, msgid, payload, crc_extra)
        self.seq = (self.seq + 1) & 0xFF
        return frame

    def heartbeat(self) -> bytes:
        mode = (MAV_MODE_FLAG_CUSTOM_MODE_ENABLED |
                MAV_MODE_FLAG_STABILIZE_ENABLED |
                MAV_MODE_FLAG_MANUAL_INPUT_ENABLED)
        payload = struct.pack(
            "<IBBBBB", 0, MAV_TYPE_QUADROTOR,
            MAV_AUTOPILOT_ARDUPILOTMEGA, mode, MAV_STATE_STANDBY, 3
        )
        return self.packet(0, payload, 50)

    def sys_status(self) -> bytes:
        sensors = 0x00000001
        payload = struct.pack(
            "<IIIHHhHHHHHHb", sensors, sensors, sensors,
            180, 12000, 350, 0, 0, 0, 0, 0, 0, 82
        )
        return self.packet(1, payload, 124)

    def gps_raw_int(self) -> bytes:
        usec = int((time.monotonic() - self.started) * 1_000_000)
        # Dummy position near Bandung; integers follow MAVLink scaling.
        payload = struct.pack(
            "<QiiiHHHHBB", usec, -69200000, 1076100000, 725000,
            90, 140, 0, 0, 3, 12
        )
        return self.packet(24, payload, 24)

    def attitude(self) -> bytes:
        elapsed = time.monotonic() - self.started
        boot_ms = int(elapsed * 1000) & 0xFFFFFFFF
        roll = math.sin(elapsed * 0.7) * 0.12
        pitch = math.sin(elapsed * 0.45) * 0.08
        yaw = (elapsed * 0.08) % (2 * math.pi)
        payload = struct.pack("<Iffffff", boot_ms, roll, pitch, yaw,
                              0.0, 0.0, 0.08)
        return self.packet(30, payload, 39)

    def global_position(self) -> bytes:
        elapsed = time.monotonic() - self.started
        boot_ms = int(elapsed * 1000) & 0xFFFFFFFF
        payload = struct.pack(
            "<IiiiihhhH", boot_ms, -69200000, 1076100000,
            725000, 10000, 0, 0, 0, 0
        )
        return self.packet(33, payload, 104)

    def vfr_hud(self) -> bytes:
        payload = struct.pack("<ffffhH", 0.0, 0.0, 725.0, 0.0, 0, 0)
        return self.packet(74, payload, 20)


def open_serial_without_reset(port_name: str, baud: int) -> serial.Serial:
    """Open CH9102 without the DTR/RTS pulse that resets an ESP32."""
    port = serial.Serial(port=None, baudrate=baud, timeout=0)
    port.port = port_name
    port.dtr = False
    port.rts = False
    port.open()
    return port


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Send dummy MAVLink through a USB-powered TelemJaya Air"
    )
    parser.add_argument("--port", default="COM13", help="Air COM port")
    parser.add_argument("--baud", type=int, default=57600)
    parser.add_argument("--boot-wait", type=float, default=8.0,
                        help="seconds to let Air finish booting")
    parser.add_argument("--ground-port",
                        help="optional Ground COM port for terminal-only path test")
    args = parser.parse_args()

    print(f"Opening Air on {args.port} at {args.baud} baud...")
    try:
        port = open_serial_without_reset(args.port, args.baud)
    except serial.SerialException as exc:
        print(f"ERROR: cannot open {args.port}: {exc}", file=sys.stderr)
        print("Close Arduino Serial Monitor and other programs using Air COM.")
        return 2

    ground = None
    if args.ground_port:
        print(f"Opening Ground on {args.ground_port} at {args.baud} baud...")
        try:
            ground = open_serial_without_reset(args.ground_port, args.baud)
        except serial.SerialException as exc:
            port.close()
            print(f"ERROR: cannot open {args.ground_port}: {exc}", file=sys.stderr)
            print("Close Mission Planner and Serial Monitor first.")
            return 3

    # Avoid intentionally holding the ESP32 in reset. Some USB-UART drivers may
    # still pulse reset once when the COM port opens, hence the boot wait.
    port.dtr = False
    port.rts = False
    print(f"Waiting {args.boot_wait:g}s for Air firmware...")
    time.sleep(args.boot_wait)
    port.reset_input_buffer()
    if ground:
        ground.reset_input_buffer()

    vehicle = DummyVehicle()
    next_heartbeat = 0.0
    next_status = 0.0
    next_fast = 0.0
    tx_frames = 0
    tx_bytes = 0
    rx_bytes = 0
    ground_rx_bytes = 0
    ground_heartbeats = 0
    ground_buffer = bytearray()
    report_at = time.monotonic() + 1.0

    print("Dummy vehicle active. Keep this terminal open; CTRL-C stops it.")
    if ground:
        print("Terminal path test active; keep Mission Planner CLOSED.")
        print("PASS requires Ground bytes and valid heartbeat counts to rise.")
    else:
        print("Now connect Mission Planner to Ground COM20 at 57600.")

    try:
        while True:
            now = time.monotonic()
            frames = []
            if now >= next_heartbeat:
                frames.append(vehicle.heartbeat())
                next_heartbeat = now + 1.0
            if now >= next_status:
                frames.extend((vehicle.sys_status(), vehicle.gps_raw_int(),
                               vehicle.global_position(), vehicle.vfr_hud()))
                next_status = now + 1.0
            if now >= next_fast:
                frames.append(vehicle.attitude())
                next_fast = now + 0.20

            for frame in frames:
                port.write(frame)
                tx_frames += 1
                tx_bytes += len(frame)

            waiting = port.in_waiting
            if waiting:
                # These are requests travelling back from Mission Planner via
                # Ground and RF. Count them; don't print binary garbage.
                rx_bytes += len(port.read(waiting))

            if ground:
                waiting = ground.in_waiting
                if waiting:
                    chunk = ground.read(waiting)
                    ground_rx_bytes += len(chunk)
                    ground_buffer.extend(chunk)

                # Locate and CRC-check MAVLink v1/v2 HEARTBEAT frames. mLRS
                # may rebuild an incoming v1 packet as v2 on the Ground side.
                while ground_buffer:
                    markers = [pos for value in (0xFE, 0xFD)
                               if (pos := ground_buffer.find(bytes((value,)))) >= 0]
                    if not markers:
                        ground_buffer.clear()
                        break
                    marker = min(markers)
                    if marker:
                        del ground_buffer[:marker]
                    if len(ground_buffer) < 8:
                        break
                    is_v2 = ground_buffer[0] == 0xFD
                    if is_v2 and len(ground_buffer) < 12:
                        break
                    signed = is_v2 and bool(ground_buffer[2] & 0x01)
                    frame_len = ground_buffer[1] + (12 if is_v2 else 8)
                    if signed:
                        frame_len += 13
                    if len(ground_buffer) < frame_len:
                        break
                    frame = bytes(ground_buffer[:frame_len])
                    del ground_buffer[:frame_len]
                    msgid = ((frame[7] | (frame[8] << 8) | (frame[9] << 16))
                             if is_v2 else frame[5])
                    if msgid != 0:
                        continue
                    checksum_end = frame_len - (15 if signed else 2)
                    crc = 0xFFFF
                    for byte in frame[1:checksum_end]:
                        crc = x25_accumulate(byte, crc)
                    crc = x25_accumulate(50, crc)
                    received_crc = (frame[checksum_end] |
                                    (frame[checksum_end + 1] << 8))
                    if crc == received_crc:
                        ground_heartbeats += 1

            if now >= report_at:
                if ground:
                    verdict = "PASS" if ground_heartbeats else "WAIT"
                    print(f"TX {tx_frames:5d} frames | "
                          f"Ground RX {ground_rx_bytes:7d} B | "
                          f"valid HB {ground_heartbeats:4d} | {verdict}",
                          flush=True)
                else:
                    print(f"TX {tx_frames:5d} frames / {tx_bytes:7d} B | "
                          f"return path RX {rx_bytes:7d} B", flush=True)
                report_at = now + 1.0

            time.sleep(0.005)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        port.close()
        if ground:
            ground.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
