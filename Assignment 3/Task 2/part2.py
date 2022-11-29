import matplotlib.pyplot as plt
import pandas as pd

channelDataRate = [3,5,10,15,30]
appDataRate = [1,2,4,8,12]

def plot_task2(channelDataRate, appDataRate):

	file = f'task2_channel_{channelDataRate}_app_{appDataRate}.csv'

	df = pd.read_csv(file, header = None, names = ["Time", "Old Cwnd", "New Cwnd"])
	df.plot(x = 'Time', y = 'New Cwnd', legend = False)
	plt.title(f"Channel Data Rate: {channelDataRate}, Application Data Rate: {appDataRate}")
	plt.xlabel("Time(sec)")
	plt.ylabel("Cwnd size")
	plt.savefig(f'task2_channel_{channelDataRate}_app_{appDataRate}.png')
	

for ch in channelDataRate:
	plot_task2(ch, 5)

for app in appDataRate:
	plot_task2(4, app)

	
