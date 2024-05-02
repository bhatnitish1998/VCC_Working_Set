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



