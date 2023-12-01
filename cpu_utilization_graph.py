import csv

from matplotlib import pyplot as plt


def pre_process_cpu_utilization_data(data):
    count = 1
    processed_data = []
    l = [0, 0]
    current_clients = 0
    data = list(data)
    try:
        for i in range(len(data)):
            if (data[i][1] == 'r' or data[i][1] == 'procs'):
                continue
            if (current_clients != int(data[i][0])):
                current_clients = int(data[i][0])
                l[1] = l[1] / count
                processed_data.append(l)
                count = 0
                l = [int(data[i][0]), 0]
            else:
                try:
                    l[1] += float(data[i][13]) + float(data[i][14])
                except:
                    print(data[i])
                count += 1
        l[1] = l[1] / count
        processed_data.append(l)
    except:
        pass
    return processed_data

def pre_process_threads_data(data):
    count = 0
    processed_data = []
    data = list(data)
    l = [0, 0]
    try:
        for i in data:
            if i[0] == '':
                l[1] = l[1] / count
                processed_data.append(l)
                l = [0, 0]
                count = 0
            else:
                l[0] = int(i[0])
                l[1] += int(i[1])
                count += 1
        l[1] = l[1] / count
        l[0] = int(i[0])
        processed_data.append(l)
    except:
        pass
    return processed_data



def threads_graph(data):
    plt.clf()
    plt.title("Avg Threads vs No of clients")
    plt.xlabel("No of clients.")
    plt.ylabel("Avg Threads")
    x_points = [i[0] for i in data]
    y_points = [i[1] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("threads.png")

def cpu_utilization_graph(data):
    plt.clf()
    plt.title("CPU Utilization in % vs No of clients")
    plt.xlabel("No of clients.")
    plt.ylabel("CPU Utilization in %")
    x_points = [i[0] for i in data]
    y_points = [i[1] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("cpu_utilization.png")


with open('threads.csv') as f:
    data = csv.reader(f)
    data = pre_process_threads_data(data)
    # print(data)
    threads_graph(data)


with open('cpu_utilization.csv') as f:
    data = csv.reader(f)
    data = pre_process_cpu_utilization_data(data)
    # print(data)
    cpu_utilization_graph(data)
