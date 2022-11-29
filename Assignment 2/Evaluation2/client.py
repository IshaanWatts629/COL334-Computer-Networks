from socket import *
import chunks

bufferSize = 2048
serverName = '127.0.0.1'

serverPort_initial = 11000

chunks_dict = {}

TCPClientSocket = socket(AF_INET, SOCK_STREAM)
TCPClientSocket.connect((serverName, serverPort_initial))

TCPClientSocket.send('Hello'.encode())
welcome = TCPClientSocket.recv(bufferSize)
print(welcome.decode())

while len(chunks_dict.keys()) != 10:

	for i in range(1,11):
		if i not in chunks_dict.keys():

			#print(f'Requesting {i}')

			TCPClientSocket.send(str(i).encode())

			message = TCPClientSocket.recv(bufferSize)
			message = message.decode('utf-8', 'ignore')
			message = message.split('*')

			#print(message)

			if chunks.get_hash(message[0]) == message[1]:
				chunks_dict[i] = message[0]
				#print(f'Recv chunk --> {i}, {message[0]}')


TCPClientSocket.send('end'.encode())
TCPClientSocket.close()
print('Recieved all chunks')

with open('output.txt', 'w') as file:

	s = []

	for i in range(1,11):
		s.append(chunks_dict[i])

	file.write(' '.join(s))














