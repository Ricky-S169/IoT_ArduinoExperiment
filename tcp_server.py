#! /usr/bin/env python
# coding:utf-8
# tcp_server
import socket
import threading

bind_ip = '0.0.0.0'
bind_port = 10017 # ** 割り当てられたものを使用せよ**
server = socket.socket(socket.AF_INET,socket.SOCK_STREAM) 
server.bind((bind_ip,bind_port))
server.listen(5)



print('[*] listen %s:%d' % (bind_ip,bind_port))
def handle_client(client_socket):
 i = 0
 movement = [0,0,0,0,0]
 bufsize=1024
 while True:
    request = client_socket.recv(bufsize) 
    if (request != ""): #XXX
        print('[*] recv: %s' % request)
        splitrequest = request.split(',')
        movement[i] = int(splitrequest[3])
        if (int(splitrequest[3])):
            if len(splitrequest) != 4:
                client_socket.send("ERROR\n".encode("UTF-8"))
                print("error")
            else:
                client_socket.send("OK\n".encode("UTF-8"))
                #書き込みはうまくいっている
                file = open('record.csv', 'a')
                file.write(request)
                file.close()
                print("ok")
        else:
            if len(splitrequest) != 4:
                client_socket.send("XERROR\n".encode("UTF-8"))
                print("xerror")
            else:
                client_socket.send("XOK\n".encode("UTF-8"))
                #書き込みはうまくいっている
                print("xok")
                file = open('record.csv', 'a')
                file.write(request)
                file.close()






while True:
 client,addr = server.accept()
 print('[*] connected from: %s:%d' % (addr[0],addr[1]))
 client_handler = threading.Thread(target=handle_client,args=(client,))
 client_handler.start()
