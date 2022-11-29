import matplotlib.pyplot as plt
import pandas as pd

connection = [1,2,3]
config = [1,2]

def plot(connection, config):

	file = f"task1_connection{connection}_{config}.csv"

	df = pd.read_csv(file, header = None, names = ["Time", "Old Cwnd", "New Cwnd"])
	df.plot(x = 'Time', y = 'New Cwnd', legend = False)

	plt.title(f"Connection: {connection} - Configuration: {config}")
	plt.xlabel("Time(sec)")
	plt.ylabel("Cwnd size")
	plt.savefig(f"task1_connection{connection}_{config}.png")
	
for i in connection:
	for j in config:
		plot(i, j)