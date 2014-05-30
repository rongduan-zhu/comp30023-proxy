#!/usr/bin/env python

# Multithreaded python program simulating a client's behaviour
# Author:           Maxim Lobanov & Rongduan Zhu
# Last Modified:    30/05/2014

import threading, time, sys, socket, random

BUFFER_SIZE = 10
cerr = sys.stderr.write

address = ["www.google.com",
            "www.unimelb.edu.au",
            "www.cis.unimelb.edu.au",
            "www.github.com",
            "www.facebook.com",
            "uni.tg.id.au",
            "www.mubook.me",
            "mundula.csse.unimelb.edu.au",
            "www.youtube.com",
            "www.lms.unimelb.edu.au",
            "www.worldslongestwebsite.com",
            "www.amazon.com"]

def client_thread(proxy_addr, port_no):
    try:
        sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        sockfd.connect((proxy_addr, int(port_no)))

        server_addr = address[random.randint(0, len(address) - 1)]


        query = "GET http://" + server_addr + "/ HTTP/1.0\r\n\r\n"
        while len(query) > 0:
            # pick a random number of bytes to send
            rand_int = random.randint(1, 10)
            bsend = rand_int if rand_int < len(query) else len(query)

            sockfd.send(query[:bsend])

            query = query[bsend:]

            #waits a few milliseconds
            time_waited = random.random()
            time.sleep(time_waited)

        temp_data = sockfd.recv(BUFFER_SIZE)

        data = temp_data
        while (len(temp_data) > 0):

            temp_data = sockfd.recv(BUFFER_SIZE)
            data += temp_data

        cerr(data + "\n\n\n")
        cerr("Received " + str(len(data)) + " bytes of data from " + server_addr + "\n")
        cerr("Port Number: " + str(i) + "\n")
        return
    except:
        pass

def to_quit(message, status):
    _to_quit = random.randint(1, 2)
    if _to_quit == 0:
        # cerr(message)
        return True
    # cerr("Didn't quit " + status + "\n")
    return False

class ActivePool(object):
    def __init__(self):
        super(ActivePool, self).__init__()
        self.active=[]
        self.lock=threading.Lock()
    def makeActive(self, name):
        with self.lock:
            self.active.append(name)
    def makeInactive(self, name):
        with self.lock:
            self.active.remove(name)
    def numActive(self):
        with self.lock:
            return len(self.active)
    def __str__(self):
        with self.lock:
            return str(self.active)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        cerr("usage: ./client.py proxy_addr server_port\n")
        exit(1)

    for i in xrange(1000, 100000):
        # cerr("Checking port " + str(i) + "\n")
        thread = threading.Thread(target = client_thread, args = (sys.argv[1], i))
        thread.start()
        # raw_input("Press enter to spawn next thread\n")
        # time.sleep(0.01)

    cerr("Terminated\n")

