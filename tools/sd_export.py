#!/usr/bin/env python3
import argparse
import os
import sys
import time

import serial


def read_line(ser, timeout=10.0):
    end_time = time.time() + timeout
    buf = bytearray()
    while time.time() < end_time:
        chunk = ser.read(1)
        if not chunk:
            continue
        if chunk == b"\n":
            return buf.decode("utf-8", errors="replace").strip()
        if chunk != b"\r":
            buf += chunk
    preview = buf.decode("utf-8", errors="replace")
    raise TimeoutError("read_line timeout after %.1fs (partial=%r)" % (timeout, preview))


def read_exact(ser, size, out_fp=None, idle_timeout=10.0):
    remaining = size
    last_rx = time.time()
    total_received = 0
    while remaining > 0:
        chunk = ser.read(min(4096, remaining))
        if not chunk:
            if time.time() - last_rx > idle_timeout:
                if total_received == 0:
                    raise TimeoutError("no data after FILE_BEGIN")
                raise TimeoutError("read_exact timeout after %.1fs (remaining=%d)" % (idle_timeout, remaining))
            continue
        remaining -= len(chunk)
        last_rx = time.time()
        total_received += len(chunk)
        if out_fp:
            out_fp.write(chunk)


def send_cmd(ser, line):
    ser.write((line + "\r\n").encode("utf-8"))
    ser.flush()


def ensure_dir(path):
    if not os.path.isdir(path):
        os.makedirs(path, exist_ok=True)


def path_to_local(root_out, remote_path):
    rel = remote_path.lstrip("/").replace("sdcard/", "")
    return os.path.join(root_out, rel)


def handle_begin_file(ser, line, out_dir, data_timeout, line_timeout):
    parts = line.split()
    if len(parts) < 3:
        raise RuntimeError("bad FILE_BEGIN")
    remote_path = parts[1]
    size = int(parts[2])
    local_path = path_to_local(out_dir, remote_path)
    ensure_dir(os.path.dirname(local_path))
    tmp_path = local_path + ".part"
    try:
        with open(tmp_path, "wb") as f:
            read_exact(ser, size, f, idle_timeout=data_timeout)
        end_line = read_line(ser, timeout=line_timeout)
        if end_line != "FILE_END":
            raise RuntimeError("missing FILE_END")
        os.replace(tmp_path, local_path)
        print("saved", local_path)
    except BaseException as exc:
        try:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        except OSError:
            pass
        if isinstance(exc, TimeoutError):
            try:
                line = read_line(ser, timeout=1.0)
                if line.startswith("ERR "):
                    raise RuntimeError(line) from exc
            except TimeoutError:
                pass
        raise


def handle_bundle(ser, line, out_dir, data_timeout, line_timeout):
    parts = line.split()
    if len(parts) < 2:
        raise RuntimeError("bad BEGIN_BUNDLE")
    root = parts[1]
    print("bundle", root)
    while True:
        hdr = read_line(ser, timeout=line_timeout)
        if hdr.startswith("ERR "):
            print(hdr)
            raise RuntimeError(hdr)
        if hdr.startswith(("LS ", "opendir ", "readdir ")):
            print(hdr)
            continue
        if hdr == "END_BUNDLE":
            break
        if hdr.startswith("FILE_BEGIN "):
            parts = hdr.split()
            if len(parts) < 3:
                raise RuntimeError("bad FILE_BEGIN header")
            remote_path = parts[1]
            size = int(parts[2])
            local_path = path_to_local(out_dir, remote_path)
            ensure_dir(os.path.dirname(local_path))
            tmp_path = local_path + ".part"
            try:
                with open(tmp_path, "wb") as f:
                    read_exact(ser, size, f, idle_timeout=data_timeout)
                end_line = read_line(ser, timeout=line_timeout)
                if end_line != "FILE_END":
                    raise RuntimeError("missing FILE_END in bundle")
                os.replace(tmp_path, local_path)
                print("saved", local_path)
            except BaseException as exc:
                try:
                    if os.path.exists(tmp_path):
                        os.remove(tmp_path)
                except OSError:
                    pass
                if isinstance(exc, TimeoutError):
                    try:
                        line = read_line(ser, timeout=1.0)
                        if line.startswith("ERR "):
                            raise RuntimeError(line) from exc
                    except TimeoutError:
                        pass
                raise
        else:
            raise RuntimeError("unexpected line: %s" % hdr)


def handle_list(ser, line):
    parts = line.split()
    if len(parts) < 2:
        raise RuntimeError("bad BEGIN_LIST")
    path = parts[1]
    print("list", path)
    while True:
        entry = read_line(ser, timeout=10.0)
        if entry.startswith("ERR "):
            print(entry)
            raise RuntimeError(entry)
        if entry.startswith(("LS ", "opendir ", "readdir ")):
            print(entry)
            continue
        if entry == "END_LIST":
            break
        print(entry)


def main():
    parser = argparse.ArgumentParser(description="EcoStep SD export client")
    parser.add_argument("--port", required=True, help="COM port, e.g. COM19")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--out", default="sd_export_out")
    parser.add_argument("--timeout", type=float, default=30.0, help="line timeout seconds")
    parser.add_argument("--data-timeout", type=float, default=5.0, help="data idle timeout seconds")
    sub = parser.add_subparsers(dest="cmd", required=True)

    sub.add_parser("ls").add_argument("path", nargs="?", default="/sdcard")
    sub.add_parser("get").add_argument("path")
    sub.add_parser("pull-day").add_argument("day")
    sub.add_parser("pull-dir").add_argument("path")
    sub.add_parser("pull-all")
    sub.add_parser("rm").add_argument("path")
    sub.add_parser("rmdir").add_argument("path")
    sub.add_parser("rm-rf").add_argument("path")
    sub.add_parser("clear-all")

    args = parser.parse_args()
    ensure_dir(args.out)

    with serial.Serial(args.port, args.baud, timeout=0.1) as ser:
        if args.cmd == "ls":
            send_cmd(ser, "sd_ls %s" % args.path)
        elif args.cmd == "get":
            send_cmd(ser, "sd_get %s" % args.path)
        elif args.cmd == "pull-day":
            send_cmd(ser, "sd_pull_day %s" % args.day)
        elif args.cmd == "pull-dir":
            send_cmd(ser, "sd_pull_dir %s" % args.path)
        elif args.cmd == "pull-all":
            send_cmd(ser, "sd_pull_all")
        elif args.cmd == "rm":
            send_cmd(ser, "sd_rm %s" % args.path)
        elif args.cmd == "rmdir":
            send_cmd(ser, "sd_rmdir %s" % args.path)
        elif args.cmd == "rm-rf":
            send_cmd(ser, "sd_rm_rf %s" % args.path)
        elif args.cmd == "clear-all":
            send_cmd(ser, "sd_clear_all CONFIRM")

        no_payload = args.cmd in ("rm", "rmdir", "rm-rf", "clear-all")
        try:
            while True:
                line = read_line(ser, timeout=args.timeout)
                if line.startswith("ERR "):
                    print(line)
                    return 1
                if line.startswith(("LS ", "opendir ", "readdir ")):
                    print(line)
                    continue
                if line.startswith("BEGIN_LIST "):
                    handle_list(ser, line)
                    break
                if line.startswith("FILE_BEGIN "):
                    handle_begin_file(ser, line, args.out, args.data_timeout, args.timeout)
                    break
                if line.startswith("BEGIN_BUNDLE "):
                    handle_bundle(ser, line, args.out, args.data_timeout, args.timeout)
                    break
                if line.startswith("OK "):
                    print(line)
                    if no_payload:
                        return 0
                    continue
        except RuntimeError as exc:
            print("Error: %s" % exc, file=sys.stderr)
            return 1
        except TimeoutError as exc:
            print("Timeout: %s" % exc, file=sys.stderr)
            print("No response within timeout. Check port/baud and device export state.", file=sys.stderr)
            return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
