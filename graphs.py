import csv

from matplotlib import pyplot as plt

def pre_processing(data):
    processed_data = []
    data = list(data)[1:]
    count = 0
    # "Clients No., Average response time,Throughput,Goodput,Timeout rate,Error Rate,Request Rate,Successfull response,Loop time"
    l = [0, 0, 0, 0, 0, 0, 0, 0, 0]
    n = len(l)
    for i in range(len(data)):
        if data[i][0] == '':
            l[0] = count
            processed_data.append(l)
            count = 0
            l = [0, 0, 0, 0, 0, 0, 0, 0, 0]
        else:
            for j in range(n):
                l[j] += float(data[i][j])
            count += 1
    return processed_data

def throughput_graph(data):
    plt.clf()
    plt.xlabel("No of clients.")
    plt.ylabel("Throughput")
    x_points = [i[0] for i in data]
    y_points = [i[2] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("throughput.png")
    
def response_time_graph(data):
    plt.clf()
    plt.xlabel("No of clients.")
    plt.ylabel("Avg response time")
    x_points = [i[0] for i in data]
    y_points = [i[1] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("response_time.png")

def goodput_graph(data):
    plt.clf()
    plt.xlabel("No of clients.")
    plt.ylabel("Goodput")
    x_points = [i[0] for i in data]
    y_points = [i[3] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("goodput.png")

def timeout_rate_graph(data):
    plt.clf()
    plt.xlabel("No of clients.")
    plt.ylabel("Timeout Rate")
    x_points = [i[0] for i in data]
    y_points = [i[4] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("timeout.png")

def error_rate_graph(data):
    plt.clf()
    plt.xlabel("No of clients.")
    plt.ylabel("Error Rate")
    x_points = [i[0] for i in data]
    y_points = [i[5] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("error_rate.png")

def request_rate_graph(data):
    plt.clf()
    plt.xlabel("No of clients.")
    plt.ylabel("request_rate")
    x_points = [i[0] for i in data]
    y_points = [i[6] for i in data]
    plt.plot(x_points, y_points)
    plt.savefig("request_rate.png")

with open('analysis_data.csv') as f:
    data = csv.reader(f)
    data = pre_processing(data)
    for i in data:
        print(i)

    throughput_graph(data)
    response_time_graph(data)
    goodput_graph(data)
    timeout_rate_graph(data)
    error_rate_graph(data)
    request_rate_graph(data)



