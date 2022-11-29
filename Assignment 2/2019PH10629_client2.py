from socket import *
import threading
import hashlib
import time

hash1 = hashlib.md5(open("./A2_small_file.txt", 'r').read().encode()).hexdigest()
bufferSize = 2048
serverName = "127.0.0.1"

serverPort_TCP_Req = 21001 
serverPort_TCP_Rec = 8500 

serverPort_UDP_Send = 26500

local_UDP_Send = 35001 
local_UDP_Rec = 29001

n = 5

def Client(client_id):

	counter = 0
	chunks = {}

	#Initial Handshaking + Propgating Chunk Requests
	TCPClientSocket_Req = socket(AF_INET, SOCK_STREAM)
	TCPClientSocket_Req.connect((serverName, serverPort_TCP_Req))

	TCPClientSocket_Rec = socket(AF_INET, SOCK_STREAM)
	TCPClientSocket_Rec.connect((serverName, serverPort_TCP_Rec))

	#Done to mark TCP connection sockets with client number
	welcome = f"Hello {client_id}"
	TCPClientSocket_Req.send(welcome.encode())
	TCPClientSocket_Rec.send(welcome.encode())
	
	#UDP port for Sending Missing Chunks
	UDPClientSocket_Send = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Send.bind((serverName, local_UDP_Send+client_id))

	UDPClientSocket_Rec = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Rec.bind((serverName, local_UDP_Rec+client_id))

	while(True):

		bytesAddressPair = UDPClientSocket_Rec.recvfrom(bufferSize)
		server_message = bytesAddressPair[0].decode("utf-8", "ignore")
		address = bytesAddressPair[1]

		server_message = server_message.split("*")

		if server_message[0] == "%":
			counter = int(server_message[1])
			done = f"Client {client_id}:Initial Distribution Complete, {len(chunks.keys())}"
			print(done)
			break

		else:
			key = int(server_message[0])
			value = server_message[1]

			chunks[key] = value

			flag = f"Recieved packet : {key}, Client {client_id}"
			#print(flag)

	# Sending TCP request to server for missing chunks
	def TCP_req():

		start = time.time()
		print(f"Requesting missing chunks for Client : {client_id}, {start}")

		#print(f"Requesting missing chunks for Client : {client_id}")

		while True:
			#print(client_id)
			for i in range(counter):
				if i not in chunks:

					#print(f'Client : {client_id} requesting chunk : {i}, {len(chunks.keys())}, {chunks.keys()}')

					requirement = f"{client_id}/{i}".encode()
					TCPClientSocket_Req.send(requirement)
					garabge = TCPClientSocket_Req.recv(bufferSize)

					#print(f'Client {client_id} --> Start time for chunk {i} : {time.time()}')

			#time.sleep(0.001)

			if len(chunks.keys()) == counter:
				break

		# TCPClientSocket_Req.send('-1'.encode())
		# garabge = TCPClientSocket_Req.recv(bufferSize)

		# TCPClientSocket_Req.close()

		end = time.time()
		print(f'End time for client {client_id} : {end}, Total time : {end-start}')

		with open(f'client{client_id}.txt', "w") as file:
			s = []
		
			for key in range(counter):
				s.append(chunks[key])

			file.write("".join(s))

		hash2 = hashlib.md5(open(f"./client{client_id}.txt", "r").read().encode()).hexdigest()
		print(hash2, client_id)

	# Recieving missing chunks from server via TCP
	def UDP_accept():

		#print(f"Accepting chunks for Client : {client_id}")

		while True:

			bytesAddressPair = UDPClientSocket_Rec.recvfrom(bufferSize)
			missing_file = bytesAddressPair[0].decode("utf-8", "ignore")
			address = bytesAddressPair[1]

			#print('in')

			missing_file = missing_file.split("*")
			#print(missing_file)

			try:
				key = int(missing_file[0])
				value = missing_file[1]
			except:
				#print("Error in Accept")
				#print(missing_file)
				continue

			#print(f'Client {client_id} --> End time for chunks {missing_file[0]} : {time.time()}')

			chunks[key] = value

			#print(f'Client {client_id} accepted chunk {key}, total --> {len(chunks.keys())}')

	# Accept chunk request by UDP from server and send back chunk if it exits back to server via TCP
	def TCP_accept():

		#print(f"Accepting chunk request for Client : {client_id}")

		while True:
			message = TCPClientSocket_Rec.recv(bufferSize).decode('utf-8', 'ignore')
			TCPClientSocket_Rec.send('recieved'.encode())

			message = message.split("/")

			#print(message)

			if int(message[1]) in chunks:

				#print(f'Client {client_id} sending chunk {message[1]} for Client {message[0]}')

				packet = chunks[int(message[1])]
				packet = f"{message[0]}*{message[1]}*{packet}"

				UDPClientSocket_Send.sendto(packet.encode(), (serverName, serverPort_UDP_Send + client_id))

	threads = []

	x = threading.Thread(target=UDP_accept)
	threads.append(x)
	x.start()

	x = threading.Thread(target=TCP_accept)
	threads.append(x)
	x.start()

	x = threading.Thread(target=TCP_req)
	threads.append(x)
	x.start()


threads = []
noOfThreads = n

for i in range(noOfThreads):   # for connection 1
    x = threading.Thread(target=Client, args=(i,))
    threads.append(x)
    x.start()
			



