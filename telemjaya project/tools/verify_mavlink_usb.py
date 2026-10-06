#!/usr/bin/env python3
"""Validate raw MAVLink v1 frames coming from one serial port."""

import argparse
import time
import serial


CRC_EXTRA = {
    0: 50,    # HEARTBEAT
    1: 124,   # SYS_STATUS
    30: 39,   # ATTITUDE
    33: 104,  # GLOBAL_POSITION_INT
}


def crc_accumulate(data: int, crc: int) -> int:
    tmp = data ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM20")
    parser.add_argument("--baud", type=int, default=57600)
    args = parser.parse_args()

    print(f"Opening {args.port} @ {args.baud}...")
    port = serial.Serial(port=None, baudrate=args.baud, timeout=0)
    port.port = args.port
    port.dtr = False
    port.rts = False
    port.open()

    print("Waiting for MAVLink; CTRL-C stops. Board may reboot once on open.")
    buffer = bytearray()
    raw_bytes = 0
    valid_frames = 0
    bad_crc = 0
    heartbeat = 0
    message_counts = {}
    next_report = time.monotonic() + 1.0

    try:
        while True:
            waiting = port.in_waiting
            if waiting:
                chunk = port.read(waiting)
                raw_bytes += len(chunk)
                buffer.extend(chunk)

            while buffer:
                marker = buffer.find(b"\xFE")
                if marker < 0:
                    buffer.clear()
                    break
                if marker:
                    del buffer[:marker]
                if len(buffer) < 8:
                    break
                frame_len = buffer[1] + 8
                if len(buffer) < frame_len:
                    break
                frame = bytes(buffer[:frame_len])
                del buffer[:frame_len]
                msgid = frame[5]
                crc_extra = CRC_EXTRA.get(msgid)
                if crc_extra is None:
                    continue
                crc = 0xFFFF
                for byte in frame[1:-2]:
                    crc = crc_accumulate(byte, crc)
                crc = crc_accumulate(crc_extra, crc)
                received = frame[-2] | (frame[-1] << 8)
                if crc != received:
                    bad_crc += 1
                    continue
                valid_frames += 1
                message_counts[msgid] = message_counts.get(msgid, 0) + 1
                if msgid == 0:
                    heartbeat += 1

            now = time.monotonic()
            if now >= next_report:
                verdict = "PASS" if heartbeat > 0 else "WAIT"
                print(
                    f"RX {raw_bytes:7d} B | valid {valid_frames:5d} | "
                    f"HB {heartbeat:4d} | badCRC {bad_crc:3d} | {verdict}",
                    flush=True,
                )
                next_report = now + 1.0
            time.sleep(0.005)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        port.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
