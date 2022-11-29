from socket import *
import threading
import hashlib
import time

hash1 = hashlib.md5(open("./A2_small_file.txt", 'r').read().encode()).hexdigest()

bufferSize = 2048

serverName = "127.0.0.1"

serverPort_Inital = 19011 #Initial Handshaking + Receiving Missing Chunks

serverPort_TCP_Send = 47500 #Sending Missing Chunks
#serverPort_TCP_Cache = 13000 #Cache

serverPort_UDP_Req = 28500 #Requesting server for missing chunks

local_UDP_Req = 36001 # Requesting missing chunks from server
local_UDP_Rec = 28001 # Receiving chunk request from server

n = 5

start = time.time()

def Client(client_id):

	time_dict = {}

	counter = 0 # Total packets
	chunks = {}

	#Initial Handshaking + Receiving Missing Chunks
	TCPClientSocket = socket(AF_INET, SOCK_STREAM)
	TCPClientSocket.connect((serverName, serverPort_Inital))

	#Sending Missing Chunks
	TCPClientSocket_Send = socket(AF_INET, SOCK_STREAM)
	TCPClientSocket_Send.connect((serverName, serverPort_TCP_Send))

	# TCPClientSocket_Cache = socket(AF_INET, SOCK_STREAM)
	# TCPClientSocket_Cache.connect((serverName, serverPort_TCP_Cache))

	#Done to mark TCP connection sockets with client number
	welcome = f"Hello {client_id}"
	TCPClientSocket.send(welcome.encode())
	TCPClientSocket_Send.send(welcome.encode())
	#TCPClientSocket_Cache.send(welcome.encode())

	#UDP port for requesting missing chunks
	UDPClientSocket_Req = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Req.bind((serverName, local_UDP_Req+client_id))

	UDPClientSocket_Rec = socket(family=AF_INET, type=SOCK_DGRAM)
	UDPClientSocket_Rec.bind((serverName, local_UDP_Rec+client_id))

	#print(f"Client {client_id} : Connected")

	# Initial Packet Distribution

	while(True):

		server_message = TCPClientSocket.recv(bufferSize)
		server_message = server_message.decode("utf-8", "ignore")

		server_message = server_message.split("*")

		if server_message[0] == "%":
			counter = int(server_message[1])
			done = f"Client {client_id} :Initial Distribution Complete"
			TCPClientSocket.send(done.encode())
			#print(counter)
			#TCPClientSocket.close()
			break

		else:
			key = int(server_message[0])
			value = server_message[1]

			chunks[key] = value

			flag = f"Recieved packet : {key}, Client {client_id}"
			TCPClientSocket.send(flag.encode())


	# Sending UDP request to server for missing chunks
	def UDP_req():

		#start = time.time()

		#print(f"Requesting missing chunks for Client : {client_id}, {start}")

		while True:
			for i in range(counter):
				if i not in chunks:
					#print(f'Client : {client_id} requesting chunk : {i}')
					requirement = f"{client_id}/{i}".encode()
					UDPClientSocket_Req.sendto(requirement, (serverName,serverPort_UDP_Req+client_id))
					#print(f'Client {client_id} --> Start time for chunk {i} : {time.time()}')

					time_dict[i] = time.time()

			if len(chunks.keys()) == counter:
				break

		end = time.time()

		print(f'Total time for client {client_id} : {end-start}')

		#print(f'Client {client_id} --> {time_dict}')


		with open(f'client{client_id}.txt', "w") as file:
			s = []
		
			for key in range(counter):
				s.append(chunks[key])

			file.write("".join(s))

		hash2 = hashlib.md5(open(f"./client{client_id}.txt", "r").read().encode()).hexdigest()
		print(hash2)

		#UDPClientSocket_Req.sendto(str(-1).encode(), (serverName,serverPort_UDP_Req+client_id))


	# Recieving missing chunks from server via TCP
	def TCP_accept():

		#print(f"Accepting chunks for Client : {client_id}")
		#chunks = client_dict[client_id]

		while True:
			
			missing_file = TCPClientSocket.recv(bufferSize)
			missing_file = missing_file.decode("utf-8", "ignore")

			TCPClientSocket.send("received".encode())

			missing_file = missing_file.split("*")
			#print(f'Client {client_id} --> End time for chunks {missing_file[0]} : {time.time()}')
			#print(missing_file)

			try:
				key = int(missing_file[0])
				value = missing_file[1]
			except:
				#print("Error in Accept")
				#print(missing_file)
				continue

			#print(f'Client {client_id} receievd chunk {key}, {len(chunks.keys())}')

			chunks[key] = value
			time_dict[key] = time.time()-time_dict[key]

	def TCP_accept_cache():

		print(f"Accepting cached chunks for Client : {client_id}")
		#chunks = client_dict[client_id]

		while True:
			missing_file = TCPClientSocket_Cache.recv(bufferSize)
			missing_file = missing_file.decode("utf-8", "ignore")

			TCPClientSocket_Cache.send("received".encode())

			missing_file = missing_file.split("*")
			#print(missing_file)

			try:
				key = int(missing_file[0])
				value = missing_file[1]

			except:
				print("Error in Cache")
				print(missing_file)
				continue

			chunks[key] = value

	# Accept chunk request by UDP from server and send back chunk if it exits back to server via TCP
	def UDP_accept():

		#print(f"Accepting chunk request for Client : {client_id}")

		while True:
			bytesAddressPair = UDPClientSocket_Rec.recvfrom(bufferSize)
			message = bytesAddressPair[0].decode("utf-8", "ignore")
			address = bytesAddressPair[1]

			message = message.split("/")

			if int(message[1]) in chunks:

				#print(f'Client {client_id} sending chunk {message[1]} for Client {message[0]}')

				packet = chunks[int(message[1])]
				packet = f"{message[0]}*{message[1]}*{packet}"

				TCPClientSocket_Send.send(packet.encode())
				garabge = TCPClientSocket_Send.recv(bufferSize).decode("utf-8", "ignore")
	
	threads = []

	x = threading.Thread(target=UDP_req)
	threads.append(x)
	x.start()

	x = threading.Thread(target=UDP_accept)
	threads.append(x)
	x.start()

	x = threading.Thread(target=TCP_accept)
	threads.append(x)
	x.start()

	# x = threading.Thread(target=TCP_accept_cache)
	# threads.append(x)
	# x.start()
   
threads = []
noOfThreads = n

for i in range(noOfThreads):   # for connection 1
    x = threading.Thread(target=Client, args=(i,))
    threads.append(x)
    x.start()
