import chunks
from socket import *

bufferSize = 2048
serverName = '127.0.0.1'

serverPort_initial = 11000

TCPServerSocket = socket(family=AF_INET, type=SOCK_STREAM)
TCPServerSocket.bind((serverName, serverPort_initial))

TCPServerSocket.listen(1)
print('Server up and listening')

connectionSocket, _ = TCPServerSocket.accept()
welcome = connectionSocket.recv(bufferSize)
print(welcome.decode())
connectionSocket.send('Hello client'.encode())

while True:

	client_message = connectionSocket.recv(bufferSize)

	if client_message.decode('utf-8', 'ignore') == 'end':
		connectionSocket.close()
		break

	client_message = int(client_message.decode('utf-8', 'ignore'))
	req = chunks.get_chunk(client_message)

	chunk, hashed = req
	toSend = f'{chunk}*{hashed}'
	connectionSocket.send(toSend.encode())

print('Closing server')









