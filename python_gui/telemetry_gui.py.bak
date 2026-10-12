#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
telemetry_gui.py
-----------------------------------------------------------------------------
Host Dashboard Application for Secure Real-Time Medical Telemetry (SMTF v1.0).

Receives AES-128-GCM sealed packets from STM32F411RE over USART2 (115200-8-N-1)
or generates simulated cryptographic streams in Demo Mode.

Pipeline:
  Serial/Stream -> Frame Sync (0xAA 0x55) -> Length Validation ->
  CRC16-CCITT Verification -> AES-128-GCM Authentication & Decryption ->
  Replay & Sequence Integrity Check -> Vitals Extraction & Visualization.
-----------------------------------------------------------------------------
"""

import struct
import threading
import queue
import time
import sys
import math
import random
from dataclasses import dataclass, field

import tkinter as tk
from tkinter import ttk, messagebox

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

try:
    from cryptography.hazmat.primitives.ciphers.aead import AESGCM
    from cryptography.exceptions import InvalidTag
except ImportError:
    AESGCM = None


# ----------------------------------------------------------------------- #
# Protocol Constants (Strictly matching Core/Inc/config.h & packet.h)     #
# ----------------------------------------------------------------------- #
SYNC0 = 0xAA
SYNC1 = 0x55
VERSION = 0x01

TYPE_TELEMETRY = 0x01
TYPE_DIAGNOSTIC = 0x02
TYPE_HEARTBEAT = 0x03

AAD_LEN = 7                      # sync0, sync1, version, type, seq (2), cipher_len
NONCE_LEN = 12                   # 4B session_id + 8B counter
TAG_LEN = 16
CRC_LEN = 2
HEADER_LEN = AAD_LEN + NONCE_LEN # 19 bytes

SAMPLE_FMT = "<IH10hfffH"        # timestamp (4), seq (2), ecg[10] (20), hr (4), spo2 (4), temp (4), flags (2)
SAMPLE_LEN = struct.calcsize(SAMPLE_FMT) # 40 bytes

TOTAL_FRAME_LEN = HEADER_LEN + SAMPLE_LEN + TAG_LEN + CRC_LEN # 77 bytes

# Pre-shared demonstration key (matching Core/Src/config.c)
AES_KEY = bytes(range(16)) # 0x00, 0x01, ..., 0x0F

# Alert Flags & Clinical Constants
ALERT_LEVEL_NORMAL   = 0x0000
ALERT_LEVEL_WARNING  = 0x0001
ALERT_LEVEL_CRITICAL = 0x0002
ALERT_LEVEL_MASK     = 0x0003

ALERT_FLAG_TACHYCARDIA = 0x0004
ALERT_FLAG_BRADYCARDIA = 0x0008
ALERT_FLAG_HYPOXIA     = 0x0010
ALERT_FLAG_FEVER       = 0x0020
ALERT_FLAG_HYPOTHERMIA = 0x0040
ALERT_FLAG_ARRHYTHMIA  = 0x0080
ALERT_FLAG_SIM_MODE    = 0x8000

ECG_PLOT_WINDOW = 500  # Number of ECG points displayed on chart
BAUD_RATE = 115200


def crc16_ccitt(data: bytes) -> int:
    """Matches Packet_CRC16() in firmware exactly."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


@dataclass
class TelemetrySample:
    timestamp_ms: int
    seq: int
    ecg_samples: list
    heart_rate_bpm: float
    spo2_pct: float
    temperature_c: float
    alert_flags: int
    session_id: int
    nonce_counter: int
    raw_tag: bytes
    rx_time_ms: float = field(default_factory=lambda: time.time() * 1000.0)


@dataclass
class TelemetryStats:
    frames_seen: int = 0
    frames_ok: int = 0
    crc_errors: int = 0
    auth_failures: int = 0
    replay_rejects: int = 0
    lost_packets: int = 0
    resync_events: int = 0
    last_seq: int = None
    last_counter: int = None
    last_session_id: int = None
    start_time: float = field(default_factory=time.time)


# ----------------------------------------------------------------------- #
# Serial & Protocol Reception Worker                                      #
# ----------------------------------------------------------------------- #
class SerialReceiver(threading.Thread):
    def __init__(self, port: str, out_queue: queue.Queue, stats: TelemetryStats, is_mock: bool = False):
        super().__init__(daemon=True)
        self.port_name = port
        self.out_queue = out_queue
        self.stats = stats
        self.is_mock = is_mock
        self._stop_evt = threading.Event()
        self._buf = bytearray()
        self.aesgcm = AESGCM(AES_KEY) if AESGCM else None

        # Mock simulation state
        self._mock_seq = 0
        self._mock_counter = 0
        self._mock_session_id = 0x544D5331  # "SMT1"
        self._mock_phase = 0.0
        self._mock_mode = 0

    def stop(self):
        self._stop_evt.set()

    def set_mock_mode(self, mode: int):
        self._mock_mode = mode

    def run(self):
        if self.aesgcm is None:
            self.out_queue.put(("error", "cryptography library missing. Install: pip install cryptography"))
            return

        if self.is_mock:
            self.out_queue.put(("status", "Simulated Stream (Offline Demo Mode)"))
            self._run_mock()
            return

        if serial is None:
            self.out_queue.put(("error", "pyserial missing. Install: pip install pyserial"))
            return

        try:
            ser = serial.Serial(
                port=self.port_name,
                baudrate=BAUD_RATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.05
            )
            ser.reset_input_buffer()
        except Exception as exc:
            self.out_queue.put(("error", f"Cannot open {self.port_name}: {exc}"))
            return

        self.out_queue.put(("status", f"Connected to {self.port_name} @ {BAUD_RATE} baud"))

        first_data = False
        while not self._stop_evt.is_set():
            try:
                chunk = ser.read(ser.in_waiting or 1)
                if chunk:
                    if not first_data:
                        first_data = True
                        self.out_queue.put(("event", f"Receiving raw bytes from {self.port_name} ({len(chunk)} bytes)..."))
                    self._buf.extend(chunk)
                    self._process_buffer()
                else:
                    time.sleep(0.002)
            except Exception as exc:
                self.out_queue.put(("error", f"Serial read error: {exc}"))
                break

        ser.close()

    def _process_buffer(self):
        while True:
            sync_idx = self._find_sync()
            if sync_idx is None:
                # Discard stale buffer prefix if no sync byte found and buffer grows
                if len(self._buf) > 512:
                    del self._buf[:-1]
                return
            if sync_idx > 0:
                del self._buf[:sync_idx]
                self.stats.resync_events += 1

            if len(self._buf) < TOTAL_FRAME_LEN:
                return  # Wait for full 77-byte frame

            frame = bytes(self._buf[:TOTAL_FRAME_LEN])
            del self._buf[:TOTAL_FRAME_LEN]
            self._decode_frame(frame)

    def _find_sync(self):
        for i in range(len(self._buf) - 1):
            if self._buf[i] == SYNC0 and self._buf[i + 1] == SYNC1:
                return i
        return None

    def _decode_frame(self, frame: bytes):
        self.stats.frames_seen += 1

        # 1. CRC16 Verification
        body = frame[:-CRC_LEN]
        rx_crc = struct.unpack_from("<H", frame, len(frame) - CRC_LEN)[0]
        calc_crc = crc16_ccitt(body)
        if rx_crc != calc_crc:
            self.stats.crc_errors += 1
            self.out_queue.put(("event", f"CRC Error: rx=0x{rx_crc:04X} calc=0x{calc_crc:04X}"))
            return

        # 2. Header & AAD
        aad = frame[:AAD_LEN]
        sync0, sync1, ver, ptype, seq, cipher_len = struct.unpack("<BBBBHB", aad)

        # 3. Nonce & Replay Check
        nonce = frame[AAD_LEN:HEADER_LEN]
        session_id, counter = struct.unpack("<IQ", nonce)

        if self.stats.last_session_id == session_id:
            if self.stats.last_counter is not None and counter <= self.stats.last_counter:
                self.stats.replay_rejects += 1
                self.out_queue.put(("event", f"REPLAY REJECTED: Counter {counter} <= Last {self.stats.last_counter}"))
                return
            if self.stats.last_seq is not None and seq > self.stats.last_seq + 1:
                dropped = seq - self.stats.last_seq - 1
                self.stats.lost_packets += dropped
                self.out_queue.put(("event", f"PACKET LOSS: {dropped} packet(s) dropped between seq {self.stats.last_seq} and {seq}"))
        else:
            self.stats.last_session_id = session_id
            self.stats.last_counter = counter
            self.stats.last_seq = seq
            self.out_queue.put(("event", f"New Boot Session Detected: ID=0x{session_id:08X}"))

        # 4. GCM Tag Authentication & Decryption
        ciphertext_and_tag = frame[HEADER_LEN:HEADER_LEN + cipher_len + TAG_LEN]
        tag = frame[HEADER_LEN + cipher_len:HEADER_LEN + cipher_len + TAG_LEN]

        try:
            plaintext = self.aesgcm.decrypt(nonce, ciphertext_and_tag, aad)
        except InvalidTag:
            self.stats.auth_failures += 1
            self.out_queue.put(("event", f"GCM AUTH FAILED: Invalid tag on seq={seq} (tampering detected)"))
            return
        except Exception as exc:
            self.out_queue.put(("error", f"Decrypt Error: {exc}"))
            return

        # 5. Parse Telemetry Sample
        if ptype != TYPE_TELEMETRY or len(plaintext) < SAMPLE_LEN:
            return

        fields = struct.unpack_from(SAMPLE_FMT, plaintext, 0)
        sample = TelemetrySample(
            timestamp_ms=fields[0],
            seq=fields[1],
            ecg_samples=list(fields[2:12]),
            heart_rate_bpm=fields[12],
            spo2_pct=fields[13],
            temperature_c=fields[14],
            alert_flags=fields[15],
            session_id=session_id,
            nonce_counter=counter,
            raw_tag=tag
        )

        self.stats.last_counter = counter
        self.stats.last_seq = seq
        self.stats.frames_ok += 1
        self.out_queue.put(("sample", sample))

    def _run_mock(self):
        """Generates realistic encrypted frames for demonstration."""
        while not self._stop_evt.is_set():
            self._mock_seq += 1
            self._mock_counter += 1

            if self._mock_mode == 1:
                hr = 114.0 + random.uniform(-2, 2)
                spo2 = 93.0 + random.uniform(-0.5, 0.5)
                temp = 38.3 + random.uniform(-0.1, 0.1)
                flags = ALERT_LEVEL_WARNING | ALERT_FLAG_TACHYCARDIA | ALERT_FLAG_SIM_MODE
                period = 0.5
            elif self._mock_mode == 2:
                hr = 142.0 + random.uniform(-4, 4)
                spo2 = 87.5 + random.uniform(-0.8, 0.8)
                temp = 39.5 + random.uniform(-0.2, 0.2)
                flags = ALERT_LEVEL_CRITICAL | ALERT_FLAG_ARRHYTHMIA | ALERT_FLAG_HYPOXIA | ALERT_FLAG_FEVER | ALERT_FLAG_SIM_MODE
                period = 0.1
            else:
                hr = 72.0 + random.uniform(-1.5, 1.5)
                spo2 = 98.6 + random.uniform(-0.3, 0.3)
                temp = 36.8 + random.uniform(-0.05, 0.05)
                flags = ALERT_LEVEL_NORMAL
                period = 1.0

            ecg_pts = []
            for _ in range(10):
                step = 0.004 / (60.0 / hr)
                self._mock_phase = (self._mock_phase + step) % 1.0
                p = self._mock_phase
                if self._mock_mode == 2:
                    val = 1.2 * math.exp(-((p - 0.35)**2) / 0.005) - 0.35 * math.exp(-((p - 0.65)**2) / 0.015)
                else:
                    val = (0.10 * math.exp(-((p - 0.10)**2) / 0.0012)
                           - 0.12 * math.exp(-((p - 0.28)**2) / 0.0002)
                           + 1.00 * math.exp(-((p - 0.30)**2) / 0.0002)
                           - 0.20 * math.exp(-((p - 0.33)**2) / 0.0002)
                           + 0.22 * math.exp(-((p - 0.55)**2) / 0.004))
                jitter = random.uniform(-0.02, 0.02)
                ecg_pts.append(int((val + jitter) * 1000))

            sample_bytes = struct.pack(
                SAMPLE_FMT,
                int(time.time() * 1000) & 0xFFFFFFFF,
                self._mock_seq & 0xFFFF,
                *ecg_pts,
                hr, spo2, temp, flags
            )

            aesgcm = AESGCM(AES_KEY)
            nonce = struct.pack("<IQ", self._mock_session_id, self._mock_counter)
            aad = struct.pack("<BBBBHB", SYNC0, SYNC1, VERSION, TYPE_TELEMETRY, self._mock_seq & 0xFFFF, len(sample_bytes))
            sealed = aesgcm.encrypt(nonce, sample_bytes, aad)

            body = aad + nonce + sealed
            crc = crc16_ccitt(body)
            frame = body + struct.pack("<H", crc)

            self._decode_frame(frame)
            time.sleep(period)


# ----------------------------------------------------------------------- #
# GUI Application Class                                                   #
# ----------------------------------------------------------------------- #
class TelemetryGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("SecureRT Medical Telemetry Framework (SMTF v1.0) - Host Monitor")
        self.geometry("1100x780")
        self.minsize(950, 650)
        self.configure(bg="#121820")

        self.msg_queue = queue.Queue()
        self.stats = TelemetryStats()
        self.receiver = None

        self.ecg_buffer = [0] * ECG_PLOT_WINDOW
        self.last_sample_time = None
        self.mock_mode = 0

        self._apply_styles()
        self._build_ui()
        self.after(40, self._poll_queue)

    def _apply_styles(self):
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure("TFrame", background="#121820")
        style.configure("TLabel", background="#121820", foreground="#E2E8F0", font=("Segoe UI", 10))
        style.configure("Header.TLabel", font=("Segoe UI", 13, "bold"), foreground="#38BDF8")
        style.configure("CardTitle.TLabel", font=("Segoe UI", 9, "bold"), foreground="#94A3B8")
        style.configure("CardVal.TLabel", font=("Segoe UI", 20, "bold"), foreground="#FFFFFF")
        style.configure("CardUnit.TLabel", font=("Segoe UI", 9), foreground="#64748B")

        style.configure("TButton", font=("Segoe UI", 9, "bold"), background="#1E293B", foreground="#F8FAFC", borderwidth=1)
        style.map("TButton", background=[("active", "#334155")])

    def _build_ui(self):
        top_bar = ttk.Frame(self, padding=(10, 8))
        top_bar.pack(side=tk.TOP, fill=tk.X)

        ttk.Label(top_bar, text="SMTF v1.0 SECURE TELEMETRY", style="Header.TLabel").pack(side=tk.LEFT, padx=(0, 15))

        ttk.Label(top_bar, text="Port:").pack(side=tk.LEFT, padx=(5, 2))
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top_bar, textvariable=self.port_var, width=14, state="readonly")
        self.port_combo.pack(side=tk.LEFT, padx=2)
        self._refresh_ports()

        ttk.Button(top_bar, text="Refresh", command=self._refresh_ports).pack(side=tk.LEFT, padx=3)
        self.btn_connect = ttk.Button(top_bar, text="Connect Hardware", command=self._toggle_connect)
        self.btn_connect.pack(side=tk.LEFT, padx=3)

        self.btn_mock = ttk.Button(top_bar, text="Demo Stream (Mock)", command=self._toggle_mock)
        self.btn_mock.pack(side=tk.LEFT, padx=3)

        ttk.Label(top_bar, text="| Demo Mode:").pack(side=tk.LEFT, padx=(10, 2))
        self.btn_sim_cycle = ttk.Button(top_bar, text="Cycle Severity", command=self._cycle_mock_mode)
        self.btn_sim_cycle.pack(side=tk.LEFT, padx=3)

        self.status_lbl = tk.Label(top_bar, text="DISCONNECTED", font=("Segoe UI", 9, "bold"), bg="#1E293B", fg="#EF4444", padx=8, pady=2)
        self.status_lbl.pack(side=tk.RIGHT, padx=5)

        self.banner_frame = tk.Frame(self, bg="#1E293B", height=32, relief=tk.FLAT)
        self.banner_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=(2, 6))

        self.alert_badge = tk.Label(self.banner_frame, text="NORMAL", font=("Segoe UI", 10, "bold"), bg="#10B981", fg="#FFFFFF", padx=10)
        self.alert_badge.pack(side=tk.LEFT, fill=tk.Y)

        self.alert_desc = tk.Label(self.banner_frame, text="System Healthy: All physiological parameters within normal clinical baseline.", font=("Segoe UI", 9), bg="#1E293B", fg="#E2E8F0", padx=10)
        self.alert_desc.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.rate_badge = tk.Label(self.banner_frame, text="Rate: 1000 ms (1 Hz)", font=("Segoe UI", 9, "bold"), bg="#0F172A", fg="#38BDF8", padx=10)
        self.rate_badge.pack(side=tk.RIGHT, fill=tk.Y)

        cards_frame = ttk.Frame(self, padding=(10, 0))
        cards_frame.pack(side=tk.TOP, fill=tk.X)

        self.hr_val_var = tk.StringVar(value="--")
        self.spo2_val_var = tk.StringVar(value="--")
        self.temp_val_var = tk.StringVar(value="--")
        self.seq_val_var = tk.StringVar(value="--")

        self.hr_card = self._create_card(cards_frame, "HEART RATE", self.hr_val_var, "BPM", 0, "#10B981")
        self.spo2_card = self._create_card(cards_frame, "OXYGEN (SpO2)", self.spo2_val_var, "%", 1, "#0EA5E9")
        self.temp_card = self._create_card(cards_frame, "TEMPERATURE", self.temp_val_var, "°C", 2, "#F59E0B")
        self.seq_card = self._create_card(cards_frame, "PACKET SEQ", self.seq_val_var, "Count", 3, "#8B5CF6")

        center_frame = ttk.Frame(self, padding=(10, 6))
        center_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        plot_container = tk.Frame(center_frame, bg="#0F172A", bd=1, relief=tk.SOLID)
        plot_container.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 6))

        self.fig = Figure(figsize=(6.5, 3.8), dpi=100, facecolor="#0F172A")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#0B1120")
        self.ax.set_title("Real-Time Lead-II ECG Waveform (Simulated 250 Hz, Authenticated AES-128-GCM)", fontsize=9, color="#94A3B8", pad=8)
        self.ax.set_ylim(-600, 1600)
        self.ax.set_xlim(0, ECG_PLOT_WINDOW)
        self.ax.grid(True, color="#1E293B", linestyle="--", linewidth=0.6)
        self.ax.tick_params(colors="#64748B", labelsize=7)

        (self.ecg_line,) = self.ax.plot(self.ecg_buffer, linewidth=1.2, color="#10B981")

        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_container)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        side_panel = tk.Frame(center_frame, bg="#1E293B", width=340, bd=1, relief=tk.SOLID)
        side_panel.pack(side=tk.RIGHT, fill=tk.BOTH, padx=(4, 0))
        side_panel.pack_propagate(False)

        ttk.Label(side_panel, text="SECURITY & SYSTEM TELEMETRY", style="CardTitle.TLabel", background="#1E293B").pack(anchor="w", padx=10, pady=(8, 4))

        self.sec_vars = {
            "cipher": tk.StringVar(value="AES-128-GCM (mbedTLS)"),
            "session": tk.StringVar(value="Session: --"),
            "nonce": tk.StringVar(value="Nonce Counter: --"),
            "tag": tk.StringVar(value="Auth Tag: --"),
            "auth_ok": tk.StringVar(value="Auth Status: VALID"),
            "replay": tk.StringVar(value="Replay Rejects: 0"),
            "crc_err": tk.StringVar(value="CRC Errors: 0"),
            "loss": tk.StringVar(value="Packet Loss: 0 (0.0%)"),
            "fps": tk.StringVar(value="Packet Rate: 0.0 Hz"),
            "latency": tk.StringVar(value="End-to-End Latency: -- ms"),
        }

        self._add_telemetry_row(side_panel, "Cipher Algorithm", self.sec_vars["cipher"])
        self._add_telemetry_row(side_panel, "Boot Session ID", self.sec_vars["session"])
        self._add_telemetry_row(side_panel, "96-bit Nonce Counter", self.sec_vars["nonce"])
        self._add_telemetry_row(side_panel, "128-bit GCM Tag", self.sec_vars["tag"])
        self._add_telemetry_row(side_panel, "Authentication", self.sec_vars["auth_ok"], highlight="#10B981")
        self._add_telemetry_row(side_panel, "Replay Attacks Blocked", self.sec_vars["replay"])
        self._add_telemetry_row(side_panel, "CRC16 Faults", self.sec_vars["crc_err"])
        self._add_telemetry_row(side_panel, "Sequence Loss", self.sec_vars["loss"])
        self._add_telemetry_row(side_panel, "Transmission Cadence", self.sec_vars["fps"])
        self._add_telemetry_row(side_panel, "Latency (Est)", self.sec_vars["latency"])

        bottom_frame = tk.Frame(self, bg="#0F172A", height=100, bd=1, relief=tk.SOLID)
        bottom_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=10, pady=(0, 8))

        log_title = tk.Label(bottom_frame, text=" SYSTEM EVENT & SECURITY LOG", font=("Segoe UI", 8, "bold"), bg="#1E293B", fg="#94A3B8", anchor="w")
        log_title.pack(fill=tk.X)

        self.log_text = tk.Text(bottom_frame, height=4, bg="#0B1120", fg="#38BDF8", font=("Consolas", 8), relief=tk.FLAT, padx=6, pady=4)
        self.log_text.pack(fill=tk.BOTH, expand=True)

    def _create_card(self, parent, title, val_var, unit, col, color):
        frame = tk.Frame(parent, bg="#1E293B", bd=1, relief=tk.SOLID, padx=10, pady=8)
        frame.grid(row=0, column=col, sticky="nsew", padx=4)
        parent.grid_columnconfigure(col, weight=1)

        tk.Label(frame, text=title, font=("Segoe UI", 8, "bold"), bg="#1E293B", fg="#94A3B8").pack(anchor="w")
        val_lbl = tk.Label(frame, textvariable=val_var, font=("Segoe UI", 18, "bold"), bg="#1E293B", fg=color)
        val_lbl.pack(anchor="w", pady=(2, 0))
        tk.Label(frame, text=unit, font=("Segoe UI", 8), bg="#1E293B", fg="#64748B").pack(anchor="w")
        return val_lbl

    def _add_telemetry_row(self, parent, label_text, var, highlight=None):
        row = tk.Frame(parent, bg="#1E293B", pady=2)
        row.pack(fill=tk.X, padx=10)
        tk.Label(row, text=label_text + ":", font=("Segoe UI", 8), bg="#1E293B", fg="#94A3B8").pack(side=tk.LEFT)
        fg_col = highlight if highlight else "#E2E8F0"
        tk.Label(row, textvariable=var, font=("Segoe UI", 8, "bold"), bg="#1E293B", fg=fg_col).pack(side=tk.RIGHT)

    def _refresh_ports(self):
        ports = []
        if list_ports is not None:
            ports = [p.device for p in list_ports.comports()]
        self.port_combo["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def _log(self, msg: str):
        ts = time.strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{ts}] {msg}\n")
        self.log_text.see(tk.END)

    def _toggle_connect(self):
        if self.receiver is not None:
            self._stop_receiver()
            return

        port = self.port_var.get()
        if not port:
            messagebox.showwarning("No Port Selected", "Please select a serial COM port or use 'Demo Stream (Mock)' mode.")
            return

        self.stats = TelemetryStats()
        self.receiver = SerialReceiver(port, self.msg_queue, self.stats, is_mock=False)
        self.receiver.start()
        self.btn_connect.config(text="Disconnect")
        self.status_lbl.config(text="CONNECTING...", bg="#F59E0B", fg="#FFFFFF")
        self._log(f"Connecting to hardware serial port {port}...")

    def _toggle_mock(self):
        if self.receiver is not None and self.receiver.is_mock:
            self._stop_receiver()
            return

        if self.receiver is not None:
            self._stop_receiver()

        self.stats = TelemetryStats()
        self.receiver = SerialReceiver("MOCK", self.msg_queue, self.stats, is_mock=True)
        self.receiver.start()
        self.btn_mock.config(text="Stop Mock")
        self.status_lbl.config(text="DEMO STREAM ACTIVE", bg="#10B981", fg="#FFFFFF")
        self._log("Started AES-128-GCM encrypted mock demonstration stream.")

    def _cycle_mock_mode(self):
        self.mock_mode = (self.mock_mode + 1) % 3
        modes = ["NORMAL (1 Hz)", "WARNING: Tachycardia (2 Hz)", "CRITICAL: Arrhythmia/Hypoxia (10 Hz)"]
        if self.receiver and self.receiver.is_mock:
            self.receiver.set_mock_mode(self.mock_mode)
        self._log(f"Demo simulation mode switched to: {modes[self.mock_mode]}")

    def _stop_receiver(self):
        if self.receiver:
            self.receiver.stop()
            self.receiver = None
        self.btn_connect.config(text="Connect Hardware")
        self.btn_mock.config(text="Demo Stream (Mock)")
        self.status_lbl.config(text="DISCONNECTED", bg="#1E293B", fg="#EF4444")
        self._log("Telemetry stream disconnected.")

    def _poll_queue(self):
        updated_plot = False
        try:
            while True:
                kind, payload = self.msg_queue.get_nowait()
                if kind == "status":
                    self.status_lbl.config(text=payload.upper(), bg="#1E293B", fg="#10B981")
                    self._log(f"Status: {payload}")
                elif kind == "error":
                    self.status_lbl.config(text="ERROR", bg="#EF4444", fg="#FFFFFF")
                    self._log(f"ERROR: {payload}")
                elif kind == "event":
                    self._log(payload)
                elif kind == "sample":
                    self._apply_sample(payload)
                    updated_plot = True
        except queue.Empty:
            pass

        self._update_stats_display()

        if updated_plot:
            self.ecg_line.set_ydata(self.ecg_buffer)
            self.canvas.draw_idle()

        self.after(40, self._poll_queue)

    def _apply_sample(self, sample: TelemetrySample):
        self.hr_val_var.set(f"{sample.heart_rate_bpm:.1f}")
        self.spo2_val_var.set(f"{sample.spo2_pct:.1f}")
        self.temp_val_var.set(f"{sample.temperature_c:.2f}")
        self.seq_val_var.set(f"#{sample.seq}")

        self.ecg_buffer.extend(sample.ecg_samples)
        del self.ecg_buffer[: len(self.ecg_buffer) - ECG_PLOT_WINDOW]

        level = sample.alert_flags & ALERT_LEVEL_MASK
        flags = sample.alert_flags

        if level == ALERT_LEVEL_CRITICAL:
            self.alert_badge.config(text="CRITICAL", bg="#EF4444")
            self.rate_badge.config(text="Rate: 100 ms (10 Hz)", fg="#EF4444")
            reasons = []
            if flags & ALERT_FLAG_ARRHYTHMIA: reasons.append("Ventricular Arrhythmia")
            if flags & ALERT_FLAG_HYPOXIA: reasons.append("Critical Hypoxia (SpO2 < 90%)")
            if flags & ALERT_FLAG_FEVER: reasons.append("Severe Hyperthermia (> 39°C)")
            if flags & ALERT_FLAG_BRADYCARDIA: reasons.append("Severe Bradycardia (< 50 bpm)")
            if flags & ALERT_FLAG_TACHYCARDIA: reasons.append("Severe Tachycardia (> 125 bpm)")
            self.alert_desc.config(text="CRITICAL ALARM: " + ", ".join(reasons if reasons else ["Critical vital threshold breach"]), fg="#FCA5A5")
        elif level == ALERT_LEVEL_WARNING:
            self.alert_badge.config(text="WARNING", bg="#F59E0B")
            self.rate_badge.config(text="Rate: 500 ms (2 Hz)", fg="#F59E0B")
            reasons = []
            if flags & ALERT_FLAG_TACHYCARDIA: reasons.append("Elevated Heart Rate (Tachycardia)")
            if flags & ALERT_FLAG_HYPOXIA: reasons.append("Mild Hypoxia (SpO2 < 94%)")
            if flags & ALERT_FLAG_FEVER: reasons.append("Moderate Fever (> 37.8°C)")
            self.alert_desc.config(text="WARNING: " + ", ".join(reasons if reasons else ["Vital parameter excursion observed"]), fg="#FDE68A")
        else:
            self.alert_badge.config(text="NORMAL", bg="#10B981")
            self.rate_badge.config(text="Rate: 1000 ms (1 Hz)", fg="#38BDF8")
            self.alert_desc.config(text="System Healthy: All physiological parameters within normal clinical baseline.", fg="#E2E8F0")

        self.sec_vars["session"].set(f"0x{sample.session_id:08X}")
        self.sec_vars["nonce"].set(f"#{sample.nonce_counter}")
        tag_hex = sample.raw_tag.hex().upper()
        self.sec_vars["tag"].set(f"{tag_hex[:8]}...{tag_hex[-8:]}")
        self.sec_vars["auth_ok"].set("AUTHENTICATED (Tag OK)")

    def _update_stats_display(self):
        s = self.stats
        elapsed = max(time.time() - s.start_time, 0.1)
        fps = s.frames_ok / elapsed

        total_expected = s.frames_ok + s.lost_packets
        loss_pct = (s.lost_packets / total_expected * 100.0) if total_expected > 0 else 0.0

        self.sec_vars["fps"].set(f"{fps:.1f} pkts/sec")
        self.sec_vars["loss"].set(f"{s.lost_packets} ({loss_pct:.1f}%)")
        self.sec_vars["replay"].set(f"{s.replay_rejects} rejected")
        self.sec_vars["crc_err"].set(f"{s.crc_errors} faults")

    def destroy(self):
        self._stop_receiver()
        super().destroy()


if __name__ == "__main__":
    app = TelemetryGUI()
    app.mainloop()
