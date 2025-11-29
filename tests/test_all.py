#!/usr/bin/env python3
import os
import socket
import subprocess
import time
import select
import unittest
import sys


def recv_until(sock, timeout=2.0):
    sock.setblocking(0)
    data = b""
    end = time.time() + timeout
    while time.time() < end:
        ready = select.select([sock], [], [], 0.1)[0]
        if ready:
            try:
                chunk = sock.recv(4096)
            except Exception:
                break
            if not chunk:
                break
            data += chunk
        else:
            continue
    return data.decode(errors="ignore")


class IRCClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = socket.create_connection((host, port))

    def send(self, line: str):
        if not line.endswith('\r\n'):
            line = line + '\r\n'
        self.sock.sendall(line.encode())

    def recv(self, timeout=2.0):
        return recv_until(self.sock, timeout=timeout)

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass


class TestIRCServer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server_bin = os.environ.get('IRC_SERVER_BIN', './ircserv')
        cls.port = int(os.environ.get('IRC_SERVER_PORT', '6667'))
        cls.password = os.environ.get('IRC_SERVER_PASSWORD', 'testpass')
        # start server
        cls.proc = subprocess.Popen([cls.server_bin, str(cls.port), cls.password], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        # wait for listen
        end = time.time() + 5.0
        ok = False
        while time.time() < end:
            try:
                s = socket.create_connection(('127.0.0.1', cls.port), timeout=0.5)
                s.close()
                ok = True
                break
            except Exception:
                time.sleep(0.1)
        if not ok:
            cls.tearDownClass()
            raise RuntimeError('Server did not start')

    @classmethod
    def tearDownClass(cls):
        try:
            cls.proc.terminate()
            cls.proc.wait(timeout=1)
        except Exception:
            try:
                cls.proc.kill()
            except Exception:
                pass

    def setUp(self):
        self.c = IRCClient('127.0.0.1', self.port)

    def tearDown(self):
        try:
            self.c.close()
        except Exception:
            pass

    def auth_and_user(self, client: IRCClient, nick='dev', user='devuser'):
        client.send('PASS ' + self.password)
        time.sleep(0.05)
        client.send('USER ' + user)
        time.sleep(0.05)
        # optional NICK
        client.send('NICK ' + nick)
        time.sleep(0.05)
        return client.recv(timeout=0.5)

    def test_ping_pong(self):
        self.c.send('PASS ' + self.password)
        self.c.send('USER u_ping')
        time.sleep(0.05)
        self.c.send('PING')
        data = self.c.recv(timeout=1.0)
        self.assertIn('PONG', data)

    def test_list_user_and_nick(self):
        # connect two clients
        c1 = IRCClient('127.0.0.1', self.port)
        c2 = IRCClient('127.0.0.1', self.port)
        try:
            c1.send('PASS ' + self.password)
            c1.send('USER alice')
            c1.send('NICK alice_n')
            time.sleep(0.05)

            c2.send('PASS ' + self.password)
            c2.send('USER bob')
            c2.send('NICK bob_n')
            time.sleep(0.05)

            # ask for list users from c1
            c1.send('LIST_USER ')
            out = c1.recv(timeout=0.5)
            self.assertIn('alice', out)
            self.assertIn('bob', out)
        finally:
            c1.close(); c2.close()

    def test_whisper(self):
        c1 = IRCClient('127.0.0.1', self.port)
        c2 = IRCClient('127.0.0.1', self.port)
        try:
            c1.send('PASS ' + self.password)
            c1.send('USER user1')
            time.sleep(0.05)
            c2.send('PASS ' + self.password)
            c2.send('USER user2')
            time.sleep(0.05)

            # ensure server processed
            c1.recv(timeout=0.2)
            c2.recv(timeout=0.2)

            c1.send('WHISPER user2 Hello there')
            # recipient should receive the whisper
            out = c2.recv(timeout=1.0)
            self.assertIn('user1: Hello there', out)
        finally:
            c1.close(); c2.close()

    def test_whisper_various(self):
        # 5 variations: normal, non-existent target, empty message, long message, multiple messages
        # normal
        c1 = IRCClient('127.0.0.1', self.port)
        c2 = IRCClient('127.0.0.1', self.port)
        try:
            c1.send('PASS ' + self.password); c1.send('USER w1'); time.sleep(0.05)
            c2.send('PASS ' + self.password); c2.send('USER w2'); time.sleep(0.05)
            c1.recv(0.1); c2.recv(0.1)

            c1.send('WHISPER w2 hey')
            self.assertIn('w1: hey', c2.recv(timeout=0.8))

            # non-existent target
            c1.send('WHISPER nobody hi')
            # no client should receive this; ensure c2 didn't receive extra
            self.assertNotIn('nobody', c2.recv(timeout=0.5))

            # empty message
            c1.send('WHISPER w2 ')
            self.assertIn('w1: ', c2.recv(timeout=0.8))

            # long message
            longmsg = 'x' * 800
            c1.send('WHISPER w2 ' + longmsg)
            self.assertIn('w1: ' + longmsg[:50], c2.recv(timeout=1.0))

            # multiple quick messages
            for i in range(3):
                c1.send('WHISPER w2 msg' + str(i))
            out = c2.recv(timeout=1.5)
            for i in range(3):
                self.assertIn('w1: msg' + str(i), out)
        finally:
            c1.close(); c2.close()

    def test_invite_various(self):
        # 5 invite related cases
        inviter = IRCClient('127.0.0.1', self.port)
        target = IRCClient('127.0.0.1', self.port)
        try:
            inviter.send('PASS ' + self.password); inviter.send('USER inv1'); time.sleep(0.05)
            target.send('PASS ' + self.password); target.send('USER inv2'); time.sleep(0.05)
            inviter.recv(0.1); target.recv(0.1)

            # invite existing user
            inviter.send('INVITE inv2')
            out = target.recv(timeout=1.0)
            self.assertIn('has invited you to join the channel', out)

            # invite twice
            inviter.send('INVITE inv2'); inviter.send('INVITE inv2')
            out = target.recv(timeout=1.0)
            # two invites may come; ensure at least one present
            self.assertIn('has invited you to join the channel', out)

            # invite non-existent user (should not crash)
            inviter.send('INVITE nobody')
            time.sleep(0.2)
            # sanity check: server still responds to PING
            inviter.send('PING')
            self.assertIn('PONG', inviter.recv(timeout=0.8))

            # invite contains channel name
            inviter.send('INVITE inv2')
            out = target.recv(timeout=1.0)
            self.assertIn('(Press y or n)', out)

            # invite then respond 'n' (refuse) and ensure inviter gets "invite refused"
            inviter.send('INVITE inv2')
            target.recv(timeout=0.8)
            target.send('n')
            # inviter should get "invite refused" sometime
            self.assertIn('invite refused', inviter.recv(timeout=1.0))
        finally:
            inviter.close(); target.close()

    def test_mode_and_join_various(self):
        # test MODE -i blocks join, MODE -t topic restriction, MODE -o toggles op
        a = IRCClient('127.0.0.1', self.port)
        b = IRCClient('127.0.0.1', self.port)
        try:
            a.send('PASS ' + self.password); a.send('USER alice'); time.sleep(0.05)
            b.send('PASS ' + self.password); b.send('USER bob'); time.sleep(0.05)
            a.recv(0.1); b.recv(0.1)

            # join #retrocomputing
            a.send('JOIN #retrocomputing')
            out = a.recv(timeout=0.5)
            self.assertIn('Channel changed', out)

            # set invite-only on channel
            a.send('MODE -i')
            out = a.recv(timeout=0.5)
            self.assertIn('invite only', out)

            # b tries to join and should fail
            b.send('JOIN #retrocomputing')
            outb = b.recv(timeout=0.5)
            self.assertIn('Channel change failed', outb)

            # unset invite
            a.send('MODE -i')
            a.recv(timeout=0.5)
            b.send('JOIN #retrocomputing')
            self.assertIn('Channel changed', b.recv(timeout=0.8))

            # topic operator-only
            a.send('MODE -t')
            a.recv(timeout=0.5)
            # bob (not op) tries to set topic
            b.send('TOPIC newtopic')
            self.assertIn('could not be changed', b.recv(timeout=0.5))

            # make bob op
            a.send('MODE -o bob')
            outa = a.recv(timeout=0.5)
            self.assertIn('is now an operator', outa)
            # now bob sets topic
            b.send('TOPIC op-topic')
            self.assertIn('channel topic changed', b.recv(timeout=0.5))

        finally:
            a.close(); b.close()

    def test_kick_various(self):
        # kick requires operator privileges; create three clients
        op = IRCClient('127.0.0.1', self.port)
        victim = IRCClient('127.0.0.1', self.port)
        other = IRCClient('127.0.0.1', self.port)
        try:
            op.send('PASS ' + self.password); op.send('USER op'); time.sleep(0.05)
            victim.send('PASS ' + self.password); victim.send('USER vic'); time.sleep(0.05)
            other.send('PASS ' + self.password); other.send('USER oth'); time.sleep(0.05)
            op.recv(0.1); victim.recv(0.1); other.recv(0.1)

            # join same channel
            op.send('JOIN #Coffebreak'); victim.send('JOIN #Coffebreak'); other.send('JOIN #Coffebreak')
            time.sleep(0.1)
            op.recv(0.2); victim.recv(0.2); other.recv(0.2)

            # make op an operator for channel
            op.send('MODE -o oth')
            # server responds
            _ = op.recv(timeout=0.5)

            # kick victim
            op.send('KICK vic')
            # victim should receive kicked message
            self.assertIn('you have been kicked', victim.recv(timeout=1.0))
            # op should receive confirmation
            self.assertIn('purgatory', op.recv(timeout=1.0))

        finally:
            op.close(); victim.close(); other.close()

    def test_nick_user_various(self):
        # duplicate name checks
        c1 = IRCClient('127.0.0.1', self.port)
        c2 = IRCClient('127.0.0.1', self.port)
        try:
            c1.send('PASS ' + self.password); c1.send('USER dup'); time.sleep(0.05)
            c1.send('NICK dupn')
            time.sleep(0.05)
            c2.send('PASS ' + self.password); c2.send('USER dup'); time.sleep(0.05)
            # c2 attempting to set same username should receive 'unabailable' message
            out = c2.recv(timeout=0.5)
            self.assertIn('unabailable', out)
        finally:
            c1.close(); c2.close()

    def test_list_channel(self):
        c = IRCClient('127.0.0.1', self.port)
        try:
            c.send('PASS ' + self.password); c.send('USER lister'); time.sleep(0.05)
            c.send('LIST_CHANNEL ')
            out = c.recv(timeout=0.5)
            # check known channels from Server::createChannel
            self.assertIn('#General', out)
            self.assertIn('#Coffebreak', out)
        finally:
            c.close()


if __name__ == '__main__':
    unittest.main()
