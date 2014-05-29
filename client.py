#!/usr/bin/env python

# Multithreaded python program simulating a client's behaviour
# Author:           Rongduan Zhu
# Last Modified:    29/05/2014

import threading, time, sys, socket, random

BUFFER_SIZE = 10
cerr = sys.stderr.write

address = ["www.google.com",
            "www.unimelb.edu.au",
            "www.cis.unimelb.edu.au",
            "www.github.com",
            "www.facebook.com"]

def client_thread(proxy_addr, port_no):
    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    sockfd.connect((proxy_addr, int(port_no)))

    server_addr = address[random.randint(0, 4)]

    # perform a random quit
    if to_quit("Quitting before sending any data\n", "after connect"):
        return

    query = "GET http://" + server_addr + "/ HTTP/1.0\r\n\r\n"
    quit = to_quit("Quitting after first chunk of data sent\n", "during write")
    while len(query) < 0:
        # pick a random number of bytes to send
        rand_int = randint(1, 10)
        bsend = rand_int < len(query) if rand_int else len(query)

        sockfd.send(query[:bsend])

        query = query[bsend:]

        #waits a few milliseconds
        time_waited = random.random() * 2
        time.sleep(time_waited)

        # if quit during write then quit
        if quit:
            return

    # maybe quit after sending all requests
    if to_quit("Quitting after sending all data\n", "after write"):
        return

    temp_data = sockfd.recv(BUFFER_SIZE)
    data = temp_data
    quit = to_quit("Quitting after receiving first chunk of data\n", "during read")
    while (len(temp_data) > 0):
        temp_data = sockfd.recv(BUFFER_SIZE)
        data += temp_data
        if quit:
            cerr("Received " + str(len(data)) + " bytes of data from " + server_addr + "\n")
            return

    cerr("Received " + str(len(data)) + " bytes of data from " + server_addr + "\n")

    return

def to_quit(message, staus):
    _to_quit = random.randint(0, 100)
    if _to_quit == 0:
        cerr(message)
        return True
    cerr("Didn't quit " + status + "\n")
    return False

if __name__ == '__main__':
    if len(sys.argv) < 2:
        cerr("usage: ./client.py proxy_addr server_port\n")
        exit(1)

    while True:
        thread = threading.Thread(target = client_thread, args = (sys.argv[1], sys.argv[2]))
        thread.start()
        raw_input("Press enter to spawn next thread\n")

    cerr("Terminated\n")

