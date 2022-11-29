import matplotlib.pyplot as plt
import pandas as pd

files = ['NewReno', 'WestWood', 'Veno', 'Vegas']

def plot_task1(protocol):
	df = pd.read_csv(f'{protocol}.csv', header = None, names = ["Time", "Old Cwnd", "New Cwnd"])
	df.plot(x = 'Time', y = 'New Cwnd', legend = False)
	plt.xlabel("Time(sec)")
	plt.ylabel("Cwnd size")
	plt.title(f'{protocol}, Max Window Size --> {max(df["New Cwnd"])}')
	plt.savefig(f'{protocol}.png')

for file in files:
	plot_task1(file)
