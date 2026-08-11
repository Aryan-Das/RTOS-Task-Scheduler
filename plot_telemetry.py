import serial
import re
import time
from collections import deque, defaultdict
import matplotlib.pyplot as plt
import matplotlib.animation as animation

PORT = '/dev/tty.usbserial-BH00LY16'
BAUD = 9600
WINDOW_SECONDS = 10
SAMPLE_RATE_HZ = 2
MAX_POINTS = WINDOW_SECONDS * SAMPLE_RATE_HZ

ACCEL_RE = re.compile(
    r'accel x:\s*(-?\d+\.\d+)\s*y:\s*(-?\d+\.\d+)\s*z:\s*(-?\d+\.\d+)\s*\|\s*filtered:\s*(-?\d+\.\d+)'
)

STATS_RE = re.compile(
    r'(\w+)\s*\|state:\s*(\d+)\s*\|priority:\s*(\d+)\s*\|switches:\s*(\d+)\s*\|stack:\s*(\d+)/(\d+)'
)

ser = serial.Serial(PORT, BAUD, timeout=1)

t_data = deque(maxlen=MAX_POINTS)
x_data = deque(maxlen=MAX_POINTS)
y_data = deque(maxlen=MAX_POINTS)
z_data = deque(maxlen=MAX_POINTS)
f_data = deque(maxlen=MAX_POINTS)

task_stacks = {}
task_order = ['imu', 'control', 'telemetry', 'stats', 'idle']  

start_time = time.time()

fig, (ax_accel, ax_stack) = plt.subplots(2, 1, figsize=(10, 8), gridspec_kw={'height_ratios': [2, 1]})


line_x, = ax_accel.plot([], [], label='X', color='tab:red')
line_y, = ax_accel.plot([], [], label='Y', color='tab:green')
line_z, = ax_accel.plot([], [], label='Z', color='tab:blue')
line_f, = ax_accel.plot([], [], label='Filtered Z', color='black', linestyle='--')

ax_accel.set_ylim(-1.5, 1.5)
ax_accel.set_xlabel('Time (s)')
ax_accel.set_ylabel('Acceleration (g)')
ax_accel.set_title('Live Accelerometer Telemetry')
ax_accel.legend(loc='upper right')
ax_accel.grid(True, alpha=0.3)


ax_stack.set_xlim(0, 100)
ax_stack.set_xlabel('Stack Usage (%)')
ax_stack.set_title('Task Stack High-Water Mark')
bars = ax_stack.barh(task_order, [0] * len(task_order), color='tab:orange')
bar_labels = [ax_stack.text(0, i, '', va='center', fontsize=9) for i in range(len(task_order))]

def update(frame):
    while ser.in_waiting:
        raw_line = ser.readline().decode(errors='replace').strip()

        accel_match = ACCEL_RE.search(raw_line)
        if accel_match:
            x, y, z, f = map(float, accel_match.groups())
            t = time.time() - start_time
            t_data.append(t)
            x_data.append(x)
            y_data.append(y)
            z_data.append(z)
            f_data.append(f)
            continue

        stats_match = STATS_RE.search(raw_line)
        if stats_match:
            name, state, priority, switches, used, total = stats_match.groups()
            task_stacks[name] = (int(used), int(total))

   
    if t_data:
        line_x.set_data(t_data, x_data)
        line_y.set_data(t_data, y_data)
        line_z.set_data(t_data, z_data)
        line_f.set_data(t_data, f_data)
        ax_accel.set_xlim(max(0, t_data[-1] - WINDOW_SECONDS), max(WINDOW_SECONDS, t_data[-1]))

    for i, name in enumerate(task_order):
        if name in task_stacks:
            used, total = task_stacks[name]
            pct = (used / total) * 100
            bars[i].set_width(pct)
         
            bars[i].set_color('tab:red' if pct > 80 else ('tab:orange' if pct > 50 else 'tab:green'))
            bar_labels[i].set_position((pct + 2, i))
            bar_labels[i].set_text(f'{used}/{total} ({pct:.1f}%)')

    return [line_x, line_y, line_z, line_f, *bars, *bar_labels]

ani = animation.FuncAnimation(fig, update, interval=100, blit=False)
plt.tight_layout()
plt.show()