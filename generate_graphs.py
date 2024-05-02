import bisect

import matplotlib.pyplot as plt
import numpy as np
import os


def get_performance(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()[-1]  # get last line

    total_time, counter = map(int, lines.split())
    perf = int(counter / total_time)

    return perf


def read_workload(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()[1:]  # Skip first line

    size = []
    duration = []

    for line in lines:
        s, d, = map(int, line.split())
        size.append(s)
        duration.append(d)

    time = []

    cumulative_sum = 0
    for i in range(len(duration)):
        cumulative_sum += duration[i]
        time.append(cumulative_sum)

    time.insert(0, 0)
    size.insert(0, 0)

    return time, size


def read_wss_estimate(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()[1:-2]

    time = []
    wss = []

    for line in lines:
        t, w = map(int, line.split())
        time.append(t)
        wss.append(w)

    time.insert(0, 0)
    wss.insert(0, 0)

    return time, wss


def get_error(x,y,x1,y1):

    x_values = [i for i in x]
    x_values.extend(x1)
    x_values.sort()
    y_actual =[]
    for val in x_values:
        index = bisect.bisect_right(x,val)-1
        y_actual.append(y[index])

    y_pred =[]
    for val in x_values:
        index = bisect.bisect_right(x1,val)-1
        y_pred.append(y1[index])

    error =0
    for i in range(len(y_pred)):
        if y_actual[i] != 0:
            error += abs(y_pred[i]-y_actual[i])/y_actual[i]
        else:
            error += abs(y_pred[i]-y_actual[i])

    return error/100

# Generate graphs
if not os.path.exists('graphs'):
    os.makedirs('graphs')

# Performance of different methods for high memory access
no_high = get_performance("logs/no_high.txt")
no_low = get_performance("logs/no_low.txt")

am_10000_high = get_performance("logs/am_10000_high.txt")
am_10000_low = get_performance("logs/am_10000_low.txt")

im_10000_high = get_performance("logs/im_10000_high.txt")
im_10000_low = get_performance("logs/im_10000_low.txt")

labels = ['50 MB', '450 MB']
no = [no_low,no_high]
am = [am_10000_low,am_10000_high]
im = [im_10000_low,im_10000_high]

x = range(len(labels))
width = 0.2
fig, ax = plt.subplots()
rects0_high = ax.bar([i - width for i in x], no, width, label='No sampling')
rects1_high = ax.bar(x, am, width, label='Access Bit')
rects2_high = ax.bar([i + width for i in x], im, width, label='Page Invalidation')

ax.set_ylabel('Counter overflows per second')
ax.set_title('Performance:Method')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=3)

fig.tight_layout()
plt.savefig('graphs/performance_method.png')
plt.clf()


#############################
# Performance for different sample size

s100 = get_performance("logs/r_100_100.txt")
s1000 = get_performance("logs/r_100_1000.txt")
s10000 = get_performance("logs/r_100_10000.txt")

labels = ['100', '1000','10000']


x = range(len(labels))
width = 0.35
fig, ax = plt.subplots()
rects_s = ax.bar( x, [s100,s1000,s10000], width)


ax.set_ylabel('Counter overflows per second')
ax.set_title('Performance: Sample size')
ax.set_xticks(x)
ax.set_xticklabels(labels)

fig.tight_layout()
plt.savefig('graphs/performance_size.png')
plt.clf()

#############################
# Performance for different sample rate

r1 = get_performance("logs/im_1_wn.txt")
r2= get_performance("logs/im_2_wn.txt")
r5 = get_performance("logs/im_5_wn.txt")

labels = ['1', '2','5']


x = range(len(labels))
width = 0.35
fig, ax = plt.subplots()
rects_r = ax.bar( x, [r1,r2,r5], width)


ax.set_ylabel('Counter overflows per second')
ax.set_title('Performance: Sample rate')
ax.set_xticks(x)
ax.set_xticklabels(labels)

fig.tight_layout()
plt.savefig('graphs/performance_rate.png')
plt.clf()


################# Accuracy graphs


# Plot accuracy graph with respect to sample size

# 0% randomness
x0,y0 = read_workload("workload_normal.txt")
x1,y1 = read_wss_estimate("logs/r_0_100.txt")
x2,y2 = read_wss_estimate("logs/r_0_1000.txt")
x3,y3 = read_wss_estimate("logs/r_0_10000.txt")

e1=get_error(x0,y0,x1,y1)
e2=get_error(x0,y0,x2,y2)
e3=get_error(x0,y0,x3,y3)

plt.step(x0,y0,label ="Actual")
plt.step(x1, y1,label="sample size = 100 pages")
plt.step(x2, y2,label="sample size = 1000 pages")
plt.step(x3, y3,label="sample size = 10000 pages")
plt.legend()
plt.title('Memory randomness 0%')
plt.xlabel('Time')
plt.ylabel('Memory Usage (MB)')
plt.grid(True)

plt.text(0, 250, f"Error rate\n100 pg = {e1:.2f}%\n1000 pg  = {e2:.2f}\n10000 pg = {e3:.2f}%", fontsize=12)

plt.savefig('graphs/accuracy_size_r0.png')
plt.clf()

# 20% randomness
x0,y0 = read_workload("workload_normal.txt")
x1,y1 = read_wss_estimate("logs/r_20_100.txt")
x2,y2 = read_wss_estimate("logs/r_20_1000.txt")
x3,y3 = read_wss_estimate("logs/r_20_10000.txt")

e1=get_error(x0,y0,x1,y1)
e2=get_error(x0,y0,x2,y2)
e3=get_error(x0,y0,x3,y3)

plt.step(x0,y0,label ="Actual")
plt.step(x1, y1,label="sample size = 100 pages")
plt.step(x2, y2,label="sample size = 1000 pages")
plt.step(x3, y3,label="sample size = 10000 pages")
plt.legend()
plt.title('Memory randomness 20%')
plt.xlabel('Time')
plt.ylabel('Memory Usage (MB)')
plt.grid(True)

plt.text(0, 250, f"Error rate\n100 pg = {e1:.2f}%\n1000 pg  = {e2:.2f}\n10000 pg = {e3:.2f}%", fontsize=12)

plt.savefig('graphs/accuracy_size_r20.png')
plt.clf()

# 100% randomness
x0,y0 = read_workload("workload_normal.txt")
x1,y1 = read_wss_estimate("logs/r_100_100.txt")
x2,y2 = read_wss_estimate("logs/r_100_1000.txt")
x3,y3 = read_wss_estimate("logs/r_100_10000.txt")

e1=get_error(x0,y0,x1,y1)
e2=get_error(x0,y0,x2,y2)
e3=get_error(x0,y0,x3,y3)

plt.step(x0,y0,label ="Actual")
plt.step(x1, y1,label="sample size = 100 pages")
plt.step(x2, y2,label="sample size = 1000 pages")
plt.step(x3, y3,label="sample size = 10000 pages")
plt.legend()
plt.title('Memory randomness 100%')
plt.xlabel('Time')
plt.ylabel('Memory Usage (MB)')
plt.grid(True)

plt.text(0, 250, f"Error rate\n100 pg = {e1:.2f}%\n1000 pg  = {e2:.2f}\n10000 pg = {e3:.2f}%", fontsize=12)

plt.savefig('graphs/accuracy_size_r100.png')
plt.clf()


############ Accuracy with sample rate

# normal workload
x0,y0 = read_workload("workload_normal.txt")
x1,y1 = read_wss_estimate("logs/im_1_wn.txt")
x2,y2 = read_wss_estimate("logs/im_2_wn.txt")
x3,y3 = read_wss_estimate("logs/im_5_wn.txt")

e1=get_error(x0,y0,x1,y1)
e2=get_error(x0,y0,x2,y2)
e3=get_error(x0,y0,x3,y3)

plt.step(x0,y0,label ="Actual")
plt.step(x1, y1,label="sample interval = 1 sec")
plt.step(x2, y2,label="sample interval = 2 sec")
plt.step(x3, y3,label="sample interval = 5 sec")
plt.legend()
plt.title('Normal Workload')
plt.xlabel('Time')
plt.ylabel('Memory Usage (MB)')
plt.grid(True)

plt.text(0, 250, f"Error rate\n1sec = {e1:.2f}%\n2sec  = {e2:.2f}\n5sec = {e3:.2f}%", fontsize=12)

plt.savefig('graphs/accuracy_rate_normal.png')
plt.clf()


# rapid workload
x0,y0 = read_workload("workload_rapid.txt")
x1,y1 = read_wss_estimate("logs/im_1_wr.txt")
x2,y2 = read_wss_estimate("logs/im_2_wr.txt")
x3,y3 = read_wss_estimate("logs/im_5_wr.txt")

e1=get_error(x0,y0,x1,y1)
e2=get_error(x0,y0,x2,y2)
e3=get_error(x0,y0,x3,y3)

plt.step(x0,y0,label ="Actual")
plt.step(x1, y1,label="sample interval = 1 sec")
plt.step(x2, y2,label="sample interval = 2 sec")
plt.step(x3, y3,label="sample interval = 5 sec")
plt.legend()
plt.title('Rapid Workload')
plt.xlabel('Time')
plt.ylabel('Memory Usage (MB)')
plt.grid(True)

plt.text(1, 0, f"Error rate\n1sec = {e1:.2f}%\n2sec  = {e2:.2f}\n5sec = {e3:.2f}%", fontsize=12)

plt.savefig('graphs/accuracy_rate_rapid.png')
plt.clf()
