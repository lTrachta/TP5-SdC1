import tkinter as tk
from tkinter import ttk
import paramiko
import time
import threading
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# === SSH CONFIG ===
SSH_HOST = "127.0.0.1"
SSH_PORT = 7777
SSH_USER = "pi"
SSH_PASS = "raspberry"

# === Estado ===
selected_signal = 0
data_x = []
data_y = []
start_time = time.time()
running = True

# === Conexión SSH persistente ===
client = paramiko.SSHClient()
client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
client.connect(SSH_HOST, port=SSH_PORT, username=SSH_USER, password=SSH_PASS)

def set_signal(sig):
    global selected_signal, data_x, data_y, start_time
    stdin, stdout, stderr = client.exec_command(f"echo {sig} | sudo tee /sys/class/cdd/select_signal")
    stdout.read()
    selected_signal = int(sig)
    data_x.clear()
    data_y.clear()
    start_time = time.time()

def read_signal():
    stdin, stdout, stderr = client.exec_command("cat /dev/cdd_sensor")
    try:
        return int(stdout.read().decode().strip())
    except:
        return 0

def update_data():
    while running:
        val = read_signal()
        timestamp = time.time() - start_time
        data_x.append(timestamp)
        data_y.append(val)
        time.sleep(1)

def update_plot():
    ax.clear()
    ax.plot(data_x, data_y, label=f"Señal {selected_signal}")
    ax.set_xlabel("Tiempo (s)")
    ax.set_ylabel("Valor de señal")
    ax.set_title(f"Gráfico de señal {selected_signal}")
    ax.legend()
    canvas.draw()
    if running:
        root.after(1000, update_plot)

def on_select(event):
    choice = signal_var.get()
    set_signal(choice)

def on_close():
    global running
    running = False
    client.close()
    root.destroy()

# === GUI setup ===
root = tk.Tk()
root.title("Visualizador CDD")
root.protocol("WM_DELETE_WINDOW", on_close)

signal_var = tk.StringVar(value="0")
ttk.Label(root, text="Seleccionar señal:").pack()
signal_menu = ttk.Combobox(root, textvariable=signal_var, values=["0", "1"], state="readonly")
signal_menu.pack()
signal_menu.bind("<<ComboboxSelected>>", on_select)

# === Matplotlib embebido ===
fig = Figure(figsize=(6, 4))
ax = fig.add_subplot(111)
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack()

# === Lanzar tareas ===
threading.Thread(target=update_data, daemon=True).start()
root.after(1000, update_plot)
root.mainloop()