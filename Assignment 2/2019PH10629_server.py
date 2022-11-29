from socket import *
import threading
import time
from collections import OrderedDict
 
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

serverPort_Inital = 19011 #Initial Handshaking + Sending Missing Chunks
serverPort_TCP_Rec = 47500 #Receiving Missing Chunks
#serverPort_TCP_Cache = 13000

serverPort_UDP_Rec = 28500 #Recieves chunk requests from clients
serverPort_UDP_Prop = 6001 #Propgates chunk requests from clients to other clients


local_UDP_Rec = 28001 # Receiving chunk request from server

client_TCP = {} # Dictionary to map client with their TCP connection
client_TCP_Rec = {} # Dictionary to map client with their TCP_Rec 
#client_TCP_Cache = {}

server_UDP_Rec = []
server_UDP_Prop = []

lock = threading.Lock()


#Initial Handshaking + Sending Missing Chunks
TCPServerSocket = socket(family=AF_INET, type=SOCK_STREAM)
TCPServerSocket.bind((serverName, serverPort_Inital))

#Receiving Missing Chunks
TCPServerSocket_Rec = socket(family=AF_INET, type=SOCK_STREAM)
TCPServerSocket_Rec.bind((serverName, serverPort_TCP_Rec))

# TCPServerSocket_Cache = socket(family=AF_INET, type=SOCK_STREAM)
# TCPServerSocket_Cache.bind((serverName, serverPort_TCP_Cache))
 
n = 5
counter = 0
cache  = LRUCache(n)

TCPServerSocket.listen(n)
TCPServerSocket_Rec.listen(n)
#TCPServerSocket_Cache.listen(n)

#print("TCP servers up and listening")
 
for i in range(n):

	# Initial handshaking with TCP
	connectionSocket, _ = TCPServerSocket.accept()

	welcome = connectionSocket.recv(bufferSize).decode('utf-8', 'ignore')
	welcome = welcome.split(' ')
	client_TCP[int(welcome[1])] = connectionSocket

	connectionSocket_Rec, _ = TCPServerSocket_Rec.accept()

	welcome = connectionSocket_Rec.recv(bufferSize).decode('utf-8', 'ignore')
	welcome = welcome.split(' ')
	client_TCP_Rec[int(welcome[1])] = connectionSocket_Rec

	# connectionSocket_Cache, _ = TCPServerSocket_Cache.accept()

	# welcome = connectionSocket_Cache.recv(bufferSize).decode('utf-8', 'ignore')
	# welcome = welcome.split(' ')
	# client_TCP_Cache[int(welcome[1])] = connectionSocket_Cache

	# Binding UDP sockets

	UDPClientSocket_Rec = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Rec.bind((serverName, serverPort_UDP_Rec + i))
	server_UDP_Rec.append(UDPClientSocket_Rec)

	UDPClientSocket_Prop = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Prop.bind((serverName, serverPort_UDP_Prop + i))
	server_UDP_Prop.append(UDPClientSocket_Prop)

#print('Starting Initial Distribution')

with open('A2_small_file.txt', 'r') as file:

	data = file.read(1024)

	while data != "":

		client_id = counter%n
		curr_socket = client_TCP[client_id]

		packet = f"{counter}*{data}"

		curr_socket.send(packet.encode())
		garbage = curr_socket.recv(bufferSize).decode('utf-8', 'ignore')
		#print(curr_socket.recv(bufferSize).decode())

		data = file.read(1024)
		counter += 1
	#print(counter)

packet = f"%*{counter}".encode()

for i in range(n):

	curr_socket = client_TCP[i]
	curr_socket.send(packet)
	#print(curr_socket.recv(bufferSize).decode('utf-8', 'ignore'))
	#curr_socket.close()

def UDP_prop(client_id):

	while True:

		UDP_client = server_UDP_Rec[client_id]

		bytesAddressPair = UDP_client.recvfrom(bufferSize)
		message = bytesAddressPair[0].decode("utf-8", "ignore")

		request = message.split("/")

		requestedBy = int(request[0])
		requestedChunk = int(request[1])

		#print(f'Chunk {requestedChunk} requested by Client :{requestedBy}')

		val = cache.get(requestedChunk)

		if val != -1:
			with lock:
				sending_socket = client_TCP[requestedBy]
				bytesToSend = f"{requestedChunk}*{val}"
				sending_socket.send(bytesToSend.encode())
				garbage = sending_socket.recv(bufferSize).decode('utf-8', 'ignore')
	

		else:
			for j in range(n):

				if j == requestedBy:
					continue

				UDP_prop = server_UDP_Prop[j]
				#print(f'Sending Chunk {requestedChunk} request by Client :{requestedBy} to Client : {j}')
				UDP_prop.sendto(message.encode(), (serverName,local_UDP_Rec + j))

def TCP_prop(client_id):

	while True:

		curr_socket = client_TCP_Rec[client_id]

		client_message = curr_socket.recv(bufferSize)
		client_message = client_message.decode("utf-8", "ignore")
		client_message = client_message.split("*")

		received = "received"
		curr_socket.send(received.encode())

		try:
			requestedBy = int(client_message[0])
			requestedChunk = int(client_message[1])
			data = client_message[2]
		except:
			continue

		temp = cache.get(requestedChunk)

		if temp == -1:
			packet = f"{requestedChunk}*{data}"

			#print(f'Putting chunk {requestedChunk} in cache')
			cache.put(requestedChunk, data)

			with lock:
				sending_socket = client_TCP[requestedBy]
				sending_socket.send(packet.encode())
				garbage = sending_socket.recv(bufferSize).decode('utf-8', 'ignore')

	# client_TCP[client_id].close()
	# client_TCP_Rec[client_id].close()

threads = []

for i in range(n):
	x = threading.Thread(target=UDP_prop, args=(i,))
	threads.append(x)
	x.start()

for i in range(n):
	x = threading.Thread(target=TCP_prop, args=(i,))
	threads.append(x)
	x.start()


	



