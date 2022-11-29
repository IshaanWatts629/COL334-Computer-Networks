from socket import *
import threading
from collections import OrderedDict
import time

class LRUCache:
 
    def __init__(self, capacity: int):
        self.cache = OrderedDict()
        self.capacity = capacity

    def get(self, key: int) -> str:
        if key not in self.cache:
            return -1
        else:
            self.cache.move_to_end(key)
            return self.cache[key]

    def put(self, key: int, value: str) -> None:
        self.cache[key] = value
        self.cache.move_to_end(key)
        if len(self.cache) > self.capacity:
            self.cache.popitem(last = False)

bufferSize = 2048

serverName = "127.0.0.1"

serverPort_TCP_Req = 21001 
serverPort_TCP_Rec = 8500 

serverPort_UDP_Send= 26500 
serverPort_UDP_Rec = 7001 

local_UDP_Rec = 29001

client_TCP_Req = {} 
client_TCP_Rec = {} 

server_UDP_Send = []
server_UDP_Rec = []

lock = threading.Lock()

#Initial Handshaking + Propgating Chunk Requests
TCPServerSocket_Req = socket(family=AF_INET, type=SOCK_STREAM)
TCPServerSocket_Req.bind((serverName, serverPort_TCP_Req))

TCPServerSocket_Rec = socket(family=AF_INET, type=SOCK_STREAM)
TCPServerSocket_Rec.bind((serverName, serverPort_TCP_Rec))
 
n = 5
counter = 0
cache = LRUCache(n)

TCPServerSocket_Req.listen(n)
TCPServerSocket_Rec.listen(n)

print("TCP servers up and listening")
 
for i in range(n):

	# Initial handshaking with TCP
	connectionSocket_Req, _ = TCPServerSocket_Req.accept()

	welcome = connectionSocket_Req.recv(bufferSize).decode('utf-8', 'ignore')
	welcome = welcome.split(' ')
	client_TCP_Req[int(welcome[1])] = connectionSocket_Req

	connectionSocket_Rec, _ = TCPServerSocket_Rec.accept()

	welcome = connectionSocket_Rec.recv(bufferSize).decode('utf-8', 'ignore')
	welcome = welcome.split(' ')
	client_TCP_Rec[int(welcome[1])] = connectionSocket_Rec

	# Binding UDP sockets

	UDPClientSocket_Send = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Send.bind((serverName, serverPort_UDP_Send + i))
	server_UDP_Send.append(UDPClientSocket_Send)

	UDPClientSocket_Rec = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Rec.bind((serverName, serverPort_UDP_Rec + i))
	server_UDP_Rec.append(UDPClientSocket_Rec)

print('Starting Initial Distribution')

with open('A2_small_file.txt', 'r') as file:

	data = file.read(1024)

	while data != "":

		client_id = counter%n
		udp_socket = server_UDP_Rec[client_id]
		packet = f"{counter}*{data}"

		udp_socket.sendto(packet.encode(), (serverName, local_UDP_Rec + client_id))

		data = file.read(1024)
		counter += 1

packet = f"%*{counter}"

for i in range(n):
	udp_socket = server_UDP_Rec[i]
	udp_socket.sendto(packet.encode(), (serverName, local_UDP_Rec + i))


def TCP_prop(client_id):

	while True:

		TCP_client = client_TCP_Req[client_id]

		message = TCP_client.recv(bufferSize).decode("utf-8", "ignore")

		TCP_client.send("received".encode())
		#print(message)

		# if message == '-1':
		# 	print('here')
		# 	continue

		request = message.split("/")

		try:
			requestedBy = int(request[0])
			requestedChunk = int(request[1])
		except:
			print('Error', client_id)
			continue

		#print(f'Chunk {requestedChunk} requested by Client :{requestedBy}')

		val = cache.get(requestedChunk)

		if val != -1:
			with lock:
				#print('sending packet')
				sending_socket = server_UDP_Rec[requestedBy]
				bytesToSend = f"{requestedChunk}*{val}"
				sending_socket.sendto(bytesToSend.encode(), (serverName, local_UDP_Rec + requestedBy))	

		else:
			for j in range(n):

				if j == requestedBy:
					continue

				TCP_prop = client_TCP_Rec[j]
				TCP_prop.send(message.encode())
				garbage = TCP_prop.recv(bufferSize)

				#print(f'Sending Chunk {requestedChunk} request by Client :{requestedBy} to Client : {j}')			

def UDP_prop(client_id):

	while True:

		curr_socket = server_UDP_Send[client_id]

		bytesAddressPair = curr_socket.recvfrom(bufferSize)
		client_message = bytesAddressPair[0].decode("utf-8", "ignore")
		address = bytesAddressPair[1]

		client_message = client_message.split("*")

		try:
			requestedBy = int(client_message[0])
			requestedChunk = int(client_message[1])
			data = client_message[2]
		except:
			print('Error')
			continue

		temp = cache.get(requestedChunk)

		#print(f'Recieved chunk {requestedChunk} from Client {client_id}')

		if temp == -1:
			packet = f"{requestedChunk}*{data}"

			#print(f'Putting chunk {requestedChunk} in cache')
			cache.put(requestedChunk, data)

			with lock:
				sending_socket = server_UDP_Rec[requestedBy]
				sending_socket.sendto(packet.encode(), (serverName, local_UDP_Rec + requestedBy))

threads = []

for i in range(n):
	x = threading.Thread(target=TCP_prop, args=(i,))
	threads.append(x)
	x.start()

for i in range(n):
	x = threading.Thread(target=UDP_prop, args=(i,))
	threads.append(x)
	x.start()




