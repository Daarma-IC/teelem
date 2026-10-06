#!/usr/bin/env python3
"""Verify both directions of the TelemJaya LoRa bridge over Ground USB."""

import argparse
import struct
import time
import serial


CRC_EXTRA = {0: 50, 4: 237}


def crc_accumulate(data: int, crc: int) -> int:
    tmp = data ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF


def make_v1(seq: int, msgid: int, payload: bytes, extra: int) -> bytes:
    header = bytes((len(payload), seq & 0xFF, 255, 190, msgid))
    crc = 0xFFFF
    for value in header + payload:
        crc = crc_accumulate(value, crc)
    crc = crc_accumulate(extra, crc)
    return b"\xFE" + header + payload + struct.pack("<H", crc)


def make_ping(mav_seq: int, ping_seq: int) -> bytes:
    payload = struct.pack("<QIBB", int(time.time() * 1_000_000), ping_seq, 1, 1)
    return make_v1(mav_seq, 4, payload, 237)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="COM20")
    parser.add_argument("--baud", type=int, default=57600)
    args = parser.parse_args()

    port = serial.Serial(port=None, baudrate=args.baud, timeout=0)
    port.port = args.port
    port.dtr = False
    port.rts = False
    port.open()
    print(f"Bridge verifier on {args.port} @ {args.baud}; CTRL-C stops.")
    print("PASS requires both Air HEARTBEAT and returned GCS PING.")

    buffer = bytearray()
    raw_rx = valid = heartbeat = returned_ping = bad_crc = 0
    mav_seq = ping_seq = 0
    next_ping = time.monotonic() + 2.0
    next_report = time.monotonic() + 1.0

    try:
        while True:
            now = time.monotonic()
            if now >= next_ping:
                port.write(make_ping(mav_seq, ping_seq))
                mav_seq = (mav_seq + 1) & 0xFF
                ping_seq += 1
                next_ping = now + 2.0

            waiting = port.in_waiting
            if waiting:
                chunk = port.read(waiting)
                raw_rx += len(chunk)
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
                extra = CRC_EXTRA.get(msgid)
                if extra is None:
                    continue
                crc = 0xFFFF
                for value in frame[1:-2]:
                    crc = crc_accumulate(value, crc)
                crc = crc_accumulate(extra, crc)
                received = frame[-2] | (frame[-1] << 8)
                if crc != received:
                    bad_crc += 1
                    continue
                valid += 1
                if msgid == 0:
                    heartbeat += 1
                elif msgid == 4:
                    returned_ping += 1

            if now >= next_report:
                passed = heartbeat > 0 and returned_ping > 0 and bad_crc == 0
                print(
                    f"RX {raw_rx:6d} B | valid {valid:4d} | HB {heartbeat:3d} | "
                    f"PING return {returned_ping:3d} | badCRC {bad_crc:3d} | "
                    f"{'PASS-2WAY' if passed else 'WAIT'}",
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

