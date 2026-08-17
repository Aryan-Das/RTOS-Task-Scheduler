import serial
import re
import time
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation

PORT = '/dev/tty.usbserial-BH00LY16'  # update if it changes
BAUD = 9600
WINDOW_SECONDS = 10          # how much history to show
SAMPLE_RATE_HZ = 2           # ~500ms per accel/pid line
MAX_POINTS = WINDOW_SECONDS * SAMPLE_RATE_HZ

# Matches: accel x: ... y: ... z: ... | filtered: ... | pid_error: ... p: ... i: ... d: ... out: ...
ACCEL_RE = re.compile(
    r'accel x:\s*(-?\d+\.\d+)\s*y:\s*(-?\d+\.\d+)\s*z:\s*(-?\d+\.\d+)\s*\|\s*filtered:\s*(-?\d+\.\d+)'
    r'\s*\|\s*pid_error:\s*(-?\d+\.\d+)\s*p:\s*(-?\d+\.\d+)\s*i:\s*(-?\d+\.\d+)\s*d:\s*(-?\d+\.\d+)\s*out:\s*(-?\d+\.\d+)'
)

STATS_RE = re.compile(
    r'(\w+)\s*\|state:\s*(\d+)\s*\|priority:\s*(\d+)\s*\|switches:\s*(\d+)\s*\|stack:\s*(\d+)/(\d+)'
)

ser = serial.Serial(PORT, BAUD, timeout=1)

# --- Accel data ---
t_data = deque(maxlen=MAX_POINTS)
x_data = deque(maxlen=MAX_POINTS)
y_data = deque(maxlen=MAX_POINTS)
z_data = deque(maxlen=MAX_POINTS)
f_data = deque(maxlen=MAX_POINTS)

# --- PID data ---
error_data = deque(maxlen=MAX_POINTS)
p_data = deque(maxlen=MAX_POINTS)
i_data = deque(maxlen=MAX_POINTS)
d_data = deque(maxlen=MAX_POINTS)
out_data = deque(maxlen=MAX_POINTS)

# --- Stack usage (latest known per task) ---
task_stacks = {}
task_order = ['imu', 'control', 'telemetry', 'stats', 'idle', 'pid']

start_time = time.time()

fig, (ax_accel, ax_pid, ax_stack) = plt.subplots(
    3, 1, figsize=(10, 11), gridspec_kw={'height_ratios': [2, 2, 1]}
)

# --- Panel 1: Accelerometer ---
line_x, = ax_accel.plot([], [], label='X', color='tab:red')
line_y, = ax_accel.plot([], [], label='Y', color='tab:green')
line_z, = ax_accel.plot([], [], label='Z', color='tab:blue')
line_f, = ax_accel.plot([], [], label='Filtered Z', color='black', linestyle='--')

ax_accel.set_ylim(-1.5, 1.5)
ax_accel.set_ylabel('Acceleration (g)')
ax_accel.set_title('Live LIS3DSH Accelerometer Telemetry')
ax_accel.legend(loc='upper right', fontsize=8)
ax_accel.grid(True, alpha=0.3)

# --- Panel 2: PID breakdown ---

line_p, = ax_pid.plot([], [], label='P term', color='tab:orange')
line_i, = ax_pid.plot([], [], label='I term', color='tab:cyan')
line_d, = ax_pid.plot([], [], label='D term', color='tab:pink')
line_out, = ax_pid.plot([], [], label='Output', color='black', linewidth=2)
line_error, = ax_pid.plot([], [], label='Error', color='purple', linestyle=':')

ax_pid.set_ylim(-2.0, 2.0)
ax_pid.set_xlabel('Time (s)')
ax_pid.set_ylabel('PID Value')
ax_pid.set_title('PID Controller (X-axis)')
ax_pid.legend(loc='upper right', fontsize=8)
ax_pid.grid(True, alpha=0.3)

# --- Panel 3: Stack usage ---
ax_stack.set_xlim(0, 100)
ax_stack.set_xlabel('Stack Usage (%)')
ax_stack.set_title('Task Stack High-Water Mark')
bars = ax_stack.barh(task_order, [0] * len(task_order), color='tab:orange')
bar_labels = [ax_stack.text(0, i, '', va='center', fontsize=8) for i in range(len(task_order))]

def update(frame):
    while ser.in_waiting:
        raw_line = ser.readline().decode(errors='replace').strip()

        accel_match = ACCEL_RE.search(raw_line)
        if accel_match:
            x, y, z, f, err, p_t, i_t, d_t, out = map(float, accel_match.groups())
            t = time.time() - start_time

            t_data.append(t)
            x_data.append(x)
            y_data.append(y)
            z_data.append(z)
            f_data.append(f)

            error_data.append(err)
            p_data.append(p_t)
            i_data.append(i_t)
            d_data.append(d_t)
            out_data.append(out)
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

        line_error.set_data(t_data, error_data)
        line_p.set_data(t_data, p_data)
        line_i.set_data(t_data, i_data)
        line_d.set_data(t_data, d_data)
        line_out.set_data(t_data, out_data)

        xlim = (max(0, t_data[-1] - WINDOW_SECONDS), max(WINDOW_SECONDS, t_data[-1]))
        ax_accel.set_xlim(*xlim)
        ax_pid.set_xlim(*xlim)

    for i, name in enumerate(task_order):
        if name in task_stacks:
            used, total = task_stacks[name]
            pct = (used / total) * 100
            bars[i].set_width(pct)
            bars[i].set_color('tab:red' if pct > 80 else ('tab:orange' if pct > 50 else 'tab:green'))
            bar_labels[i].set_position((pct + 2, i))
            bar_labels[i].set_text(f'{used}/{total} ({pct:.1f}%)')

    return [line_x, line_y, line_z, line_f,
            line_error, line_p, line_i, line_d, line_out,
            *bars, *bar_labels]

ani = animation.FuncAnimation(fig, update, interval=100, blit=False)
plt.tight_layout()
plt.show()