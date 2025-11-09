#!/usr/bin/env python3
import os
import socket
import subprocess
import sys
import tempfile
import time
from contextlib import closing


def find_free_port():
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def wait_listen(port, timeout=2.0):
    start = time.time()
    while time.time() - start < timeout:
        with closing(socket.socket()) as s:
            try:
                s.settimeout(0.1)
                s.connect(("127.0.0.1", port))
                return True
            except Exception:
                time.sleep(0.05)
    return False


def recv_all(sock, delay=0.1):
    time.sleep(delay)
    sock.settimeout(0.2)
    data = b""
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            if len(chunk) < 4096:
                break
    except Exception:
        pass
    return data


def test_wrong_then_right_pass(port):
    s = socket.socket()
    s.connect(("127.0.0.1", port))
    s.sendall(b"PASS nope\r\n")
    r1 = recv_all(s)
    assert b"464" in r1 and b"Password incorrect" in r1, f"unexpected: {r1!r}"
    s.sendall(b"PASS pass\r\n")
    r2 = recv_all(s)
    assert b"001" in r2 and b"Welcome" in r2, f"unexpected: {r2!r}"
    s.close()


def test_auth_enforcement_and_ping_echo(port):
    s = socket.socket()
    s.connect(("127.0.0.1", port))
    s.sendall(b"PING\r\n")
    r1 = recv_all(s)
    assert b"464" in r1, f"expected auth required, got {r1!r}"
    s.sendall(b"PASS pass\r\n")
    _ = recv_all(s)
    s.sendall(b"PING\r\nECHO TEST\r\n")
    r2 = recv_all(s)
    assert b"PONG\r\n" in r2 and b"ECHO TEST\r\n" in r2, f"unexpected: {r2!r}"
    s.close()


def test_multiple_commands_one_packet(port):
    s = socket.socket()
    s.connect(("127.0.0.1", port))
    s.sendall(b"PASS pass\r\nPING\r\nPING\r\n")
    r = recv_all(s)
    assert r.count(b"PONG\r\n") >= 2, f"unexpected: {r!r}"
    s.close()


def test_partial_lines_assembly(port):
    s = socket.socket()
    s.connect(("127.0.0.1", port))
    s.sendall(b"PASS")
    time.sleep(0.05)
    s.sendall(b" pass\r\nPING")
    time.sleep(0.05)
    s.sendall(b"\r\n")
    r = recv_all(s)
    assert b"001" in r and b"PONG\r\n" in r, f"unexpected: {r!r}"
    s.close()


def test_disconnect_handling(port):
    s = socket.socket()
    s.connect(("127.0.0.1", port))
    s.sendall(b"PASS pass\r\n")
    _ = recv_all(s)
    s.close()
    time.sleep(0.1)  # allow server to process close


def test_concurrent_clients(port):
    s1 = socket.socket(); s1.connect(("127.0.0.1", port))
    s2 = socket.socket(); s2.connect(("127.0.0.1", port))
    s1.sendall(b"PASS pass\r\n")
    s2.sendall(b"PASS pass\r\n")
    _ = recv_all(s1); _ = recv_all(s2)
    s1.sendall(b"PING\r\n"); s2.sendall(b"PING\r\n")
    r1 = recv_all(s1); r2 = recv_all(s2)
    assert b"PONG\r\n" in r1 and b"PONG\r\n" in r2
    s1.close(); s2.close()


def test_nick_user_setting(port):
    s = socket.socket(); s.connect(("127.0.0.1", port))
    s.sendall(b"PASS pass\r\nNICK tester\r\nUSER example\r\n")
    r = recv_all(s)
    assert b"OK NICK\r\n" in r and b"OK USER\r\n" in r, f"unexpected: {r!r}"
    s.close()


def main():
    port = find_free_port()
    env = os.environ.copy()
    server = subprocess.Popen(["./ircserv", str(port), "pass"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        assert wait_listen(port, 2.0), "server did not start listening in time"
        test_wrong_then_right_pass(port)
        test_auth_enforcement_and_ping_echo(port)
        test_multiple_commands_one_packet(port)
        test_partial_lines_assembly(port)
        test_disconnect_handling(port)
        test_concurrent_clients(port)
        test_nick_user_setting(port)
        print("ALL TESTS PASSED")
    finally:
        server.terminate()
        try:
            server.wait(timeout=1)
        except Exception:
            server.kill()

if __name__ == "__main__":
    main()
