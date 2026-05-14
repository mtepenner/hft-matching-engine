#!/usr/bin/env python3
"""
load_tester.py — FIX order flow load tester.
Sends N NewOrderSingle messages to the FIX gateway and measures round-trip latency.
Usage: python3 scripts/load_tester.py --host localhost --port 9878 --orders 100000
"""
import argparse
import socket
import time
import statistics


SOH = b'\x01'


def fix_message(tags: dict[int, str]) -> bytes:
    body = b''
    for tag, val in tags.items():
        body += f'{tag}={val}'.encode() + SOH
    checksum = sum(body) % 256
    body += f'10={checksum:03d}'.encode() + SOH
    return body


def send_logon(sock: socket.socket, seq: int) -> None:
    msg = fix_message({
        8: 'FIX.4.2',
        35: 'A',
        34: str(seq),
        49: 'LOADTEST',
        56: 'EXCHANGE',
        52: time.strftime('%Y%m%d-%H:%M:%S'),
        98: '0',
        108: '30',
    })
    sock.sendall(msg)


def send_new_order(sock: socket.socket, seq: int, cl_ord_id: str) -> None:
    msg = fix_message({
        8: 'FIX.4.2',
        35: 'D',
        34: str(seq),
        49: 'LOADTEST',
        56: 'EXCHANGE',
        52: time.strftime('%Y%m%d-%H:%M:%S'),
        11: cl_ord_id,
        21: '1',
        55: 'AAPL',
        54: '1',
        38: '100',
        40: '2',
        44: '150.00',
        60: time.strftime('%Y%m%d-%H:%M:%S'),
    })
    sock.sendall(msg)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=9878)
    parser.add_argument('--orders', type=int, default=10000)
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args.host, args.port))
    sock.settimeout(5.0)

    send_logon(sock, seq=1)
    # Drain logon ack
    try:
        sock.recv(4096)
    except socket.timeout:
        pass

    latencies: list[float] = []
    seq = 2
    print(f'[LoadTest] Sending {args.orders} orders to {args.host}:{args.port}')
    start_total = time.perf_counter()

    for i in range(args.orders):
        cl_id = f'CL{i:08d}'
        t0 = time.perf_counter_ns()
        send_new_order(sock, seq + i, cl_id)
        try:
            sock.recv(512)  # execution report
        except socket.timeout:
            pass
        t1 = time.perf_counter_ns()
        latencies.append((t1 - t0) / 1_000)  # µs

    elapsed = time.perf_counter() - start_total
    sock.close()

    print(f'[LoadTest] {args.orders} orders in {elapsed:.2f}s  '
          f'({args.orders / elapsed:,.0f} orders/sec)')
    if latencies:
        print(f'[LoadTest] Latency (µs): '
              f'p50={statistics.median(latencies):.1f}  '
              f'p99={statistics.quantiles(latencies, n=100)[98]:.1f}  '
              f'max={max(latencies):.1f}')


if __name__ == '__main__':
    main()
