#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PCM 串口发送工具

页面①  选择串口 + PCM 文件，波特率默认 460800。
        串口设置一直可选择，随时改；点击「开始发送 / 重新开始」时
        按当前选择重新打开串口，从头发送（不循环，发一遍即结束）。
        带进度条、暂停 / 继续。
        切到本页时窗口自动调整为较小尺寸。

页面②  检测 ffmpeg，把音频文件转换成 8bit 无符号 / 46080Hz 的 PCM。
        输出目录默认脚本所在目录，文件名 = 原文件名 + .pcm（同名直接覆盖）。
        转换过程实时显示 ffmpeg 日志，完成后把 PCM 文件填到页面①。
        切到本页时窗口大小不调整。

依赖：pyserial      （pip install pyserial）
      ffmpeg（可选，用于页面②，需在 PATH 中或与本脚本同目录）
"""

import os
import sys
import time
import queue
import threading
import subprocess
import shutil
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter.scrolledtext import ScrolledText

# --------------------------------------------------------------------------
# 可选依赖
# --------------------------------------------------------------------------
try:
    import serial
    import serial.tools.list_ports
    SERIAL_OK = True
    SERIAL_ERR = ""
except Exception as _e:                      # pyserial 未安装
    SERIAL_OK = False
    SERIAL_ERR = str(_e)

# --------------------------------------------------------------------------
# 常量
# --------------------------------------------------------------------------
DEFAULT_BAUD = "460800"
SAMPLE_RATE = 46080          # 目标采样率
CHUNK_SIZE = 8192            # 每次写入串口的字节数
UI_POLL_MS = 50              # 界面轮询消息队列的间隔

### 这个功能不使用了，但是免费token不够了改不了，自己改段史山救一下
SEND_GEO = "650x420"         # 发送页窗口尺寸（切到发送页时使用）
CONV_GEO = "650x420"         # 转换页默认窗口尺寸

if getattr(sys, "frozen", False):            # PyInstaller 打包后
    APP_DIR = os.path.dirname(os.path.abspath(sys.executable))
else:
    APP_DIR = os.path.dirname(os.path.abspath(__file__))


class PcmTool:
    # ======================================================================
    # 初始化
    # ======================================================================
    def __init__(self, root: tk.Tk):
        self.root = root
        root.title("PCM 串口发送工具")
        root.geometry(SEND_GEO)               # 初始停在发送页，用较小尺寸
        root.minsize(650, 450)
        root.maxsize(650, 800)

        # --- 运行状态 ---
        self.msg_queue = queue.Queue()
        self.send_thread = None
        self.current_stop = None          # 当前会话的停止事件
        self.current_pause = None         # 当前会话的暂停事件
        self.session_id = 0               # 发送会话号，用于丢弃过期消息
        self._closing = False

        # --- 窗口尺寸记忆 ---
        self._saved_geo = CONV_GEO        # 转换页上次的尺寸

        # --- 变量 ---
        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value=DEFAULT_BAUD)
        self.pcm_path = tk.StringVar()
        self.info_var = tk.StringVar(value="未选择文件")
        self.status_var = tk.StringVar(value="就绪")

        self.audio_path = tk.StringVar()
        self.out_dir_var = tk.StringVar(value=APP_DIR)
        self.channels_var = tk.StringVar(value="单声道")
        self.ffmpeg_status_var = tk.StringVar(value="正在检测 FFmpeg ...")
        self.conv_status_var = tk.StringVar(value="就绪")
        self.ffmpeg_exe = None

        # --- 界面 ---
        self._build_ui()
        self._refresh_ports()
        self._detect_ffmpeg()

        self.pcm_path.trace_add("write", lambda *_: self._update_file_info())

        self.root.after(UI_POLL_MS, self._poll_queue)
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ======================================================================
    # 界面构建
    # ======================================================================
    def _build_ui(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=8, pady=8)

        self.tab_send = ttk.Frame(self.notebook)
        self.tab_conv = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_send, text="   ① PCM 发送   ")
        self.notebook.add(self.tab_conv, text="   ② 音频转 PCM   ")

        self._build_send_tab()
        self._build_conv_tab()

        # 页签切换时调整窗口大小
        self.notebook.bind("<<NotebookTabChanged>>", self._on_tab_changed)

    # ------------------------------ 页签切换 ------------------------------
    def _on_tab_changed(self, event=None):
        try:
            idx = self.notebook.index(self.notebook.select())
        except Exception:
            return
        if idx == 0:
            # 切到发送页：记住当前尺寸，缩小窗口
            cur = self.root.geometry()
            if cur != SEND_GEO:
                self._saved_geo = cur
            self.root.geometry(SEND_GEO)
        # 切到转换页：大小不调整（保留用户当前窗口尺寸）

    # ------------------------------ 页面① ------------------------------
    def _build_send_tab(self):
        p = self.tab_send

        # ---------- 串口设置 ----------
        lf = ttk.LabelFrame(p, text="串口设置")
        lf.pack(fill="x", padx=10, pady=(10, 6))

        ttk.Label(lf, text="串口：").grid(row=0, column=0, sticky="w",
                                          padx=(10, 4), pady=6)
        self.port_combo = ttk.Combobox(lf, textvariable=self.port_var,
                                       width=32, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="w", padx=4, pady=6)
        self.btn_refresh = ttk.Button(lf, text="刷新", width=8,
                                      command=self._refresh_ports)
        self.btn_refresh.grid(row=0, column=2, padx=8, pady=6)

        ttk.Label(lf, text="波特率：").grid(row=1, column=0, sticky="w",
                                            padx=(10, 4), pady=6)
        self.baud_entry = ttk.Entry(lf, textvariable=self.baud_var, width=34)
        self.baud_entry.grid(row=1, column=1, sticky="w", padx=4, pady=6)
        ttk.Label(lf, text="默认 460800", foreground="#888").grid(
            row=1, column=2, sticky="w", padx=8)

        # ---------- 文件选择 ----------
        lf2 = ttk.LabelFrame(p, text="PCM 文件")
        lf2.pack(fill="x", padx=10, pady=6)
        lf2.columnconfigure(1, weight=1)

        ttk.Label(lf2, text="文件：").grid(row=0, column=0, sticky="w",
                                           padx=(10, 4), pady=6)
        ttk.Entry(lf2, textvariable=self.pcm_path).grid(
            row=0, column=1, sticky="ew", padx=4, pady=6)
        ttk.Button(lf2, text="浏览...", width=8, command=self._choose_pcm).grid(
            row=0, column=2, padx=8, pady=6)

        ttk.Label(lf2, textvariable=self.info_var, foreground="#666").grid(
            row=1, column=1, sticky="w", padx=4, pady=(0, 8))

        # ---------- 控制按钮 ----------
        ctrl = ttk.Frame(p)
        ctrl.pack(fill="x", padx=10, pady=8)
        self.btn_start = ttk.Button(ctrl, text="开始发送", width=14,
                                    command=self._on_start_button)
        self.btn_start.pack(side="left", padx=4)
        self.btn_pause = ttk.Button(ctrl, text="暂停", width=12,
                                    command=self._toggle_pause, state="disabled")
        self.btn_pause.pack(side="left", padx=4)

        # ---------- 进度 ----------
        pf = ttk.Frame(p)
        pf.pack(fill="x", padx=14, pady=(6, 4))
        self.progress = ttk.Progressbar(pf, orient="horizontal",
                                        mode="determinate", maximum=100.0)
        self.progress.pack(fill="x")

        ttk.Label(p, textvariable=self.status_var, anchor="w").pack(
            fill="x", padx=14, pady=(2, 10))

    # ------------------------------ 页面② ------------------------------
    def _build_conv_tab(self):
        p = self.tab_conv

        # ---------- ffmpeg 状态 ----------
        lf = ttk.LabelFrame(p, text="FFmpeg 检测")
        lf.pack(fill="x", padx=10, pady=(10, 6))
        ttk.Label(lf, textvariable=self.ffmpeg_status_var,
                  wraplength=740, justify="left").pack(anchor="w",
                                                       padx=10, pady=8)

        # ---------- 转换参数 ----------
        lf2 = ttk.LabelFrame(p, text="转换参数")
        lf2.pack(fill="x", padx=10, pady=6)
        lf2.columnconfigure(1, weight=1)

        ttk.Label(lf2, text="音频文件：").grid(row=0, column=0, sticky="w",
                                               padx=(10, 4), pady=6)
        ttk.Entry(lf2, textvariable=self.audio_path).grid(
            row=0, column=1, sticky="ew", padx=4, pady=6)
        ttk.Button(lf2, text="浏览...", width=8,
                   command=self._choose_audio).grid(row=0, column=2,
                                                    padx=8, pady=6)

        ttk.Label(lf2, text="输出目录：").grid(row=1, column=0, sticky="w",
                                               padx=(10, 4), pady=6)
        ttk.Entry(lf2, textvariable=self.out_dir_var).grid(
            row=1, column=1, sticky="ew", padx=4, pady=6)
        ttk.Button(lf2, text="浏览...", width=8,
                   command=self._choose_outdir).grid(row=1, column=2,
                                                     padx=8, pady=6)

        ttk.Label(lf2, text="声道：").grid(row=2, column=0, sticky="w",
                                           padx=(10, 4), pady=6)
        ttk.Combobox(lf2, textvariable=self.channels_var, width=12,
                     state="readonly",
                     values=["单声道", "立体声", "保持原样"]).grid(
            row=2, column=1, sticky="w", padx=4, pady=6)

        ttk.Label(lf2,
                  text="输出格式：8 位无符号（pcm_u8）  |  采样率：46080 Hz  |  "
                       "文件名：原文件名 + .pcm（同名文件直接覆盖）",
                  foreground="#666", wraplength=740, justify="left").grid(
            row=3, column=0, columnspan=3, sticky="w", padx=10, pady=(0, 8))

        # ---------- 操作按钮 ----------
        ctrl = ttk.Frame(p)
        ctrl.pack(fill="x", padx=10, pady=(6, 4))
        self.btn_convert = ttk.Button(ctrl, text="开始转换并填入发送页",
                                      width=24, state="disabled",
                                      command=self._start_convert)
        self.btn_convert.pack(side="left", padx=4)
        ttk.Button(ctrl, text="清空日志", width=10,
                   command=self._clear_log).pack(side="left", padx=8)
        ttk.Label(ctrl, textvariable=self.conv_status_var,
                  foreground="#666").pack(side="left", padx=10)

        # ---------- ffmpeg 日志 ----------
        logf = ttk.LabelFrame(p, text="FFmpeg 日志")
        logf.pack(fill="both", expand=True, padx=10, pady=(4, 10))
        self.log_text = ScrolledText(logf, height=12, wrap="word",
                                     state="disabled", font="TkFixedFont")
        self.log_text.pack(fill="both", expand=True, padx=6, pady=6)

    # ======================================================================
    # 页面①：串口 & 发送
    # ======================================================================
    def _refresh_ports(self):
        if not SERIAL_OK:
            self.port_combo["values"] = []
            self.port_var.set("")
            self.status_var.set("未安装 pyserial：pip install pyserial")
            return

        ports = sorted(p.device for p in serial.tools.list_ports.comports())
        self.port_combo["values"] = ports
        if ports:
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0])
        else:
            self.port_var.set("")

    def _choose_pcm(self):
        path = filedialog.askopenfilename(
            title="选择 PCM 文件", initialdir=APP_DIR,
            filetypes=[("PCM 文件", "*.pcm"), ("所有文件", "*.*")])
        if path:
            self.pcm_path.set(path)

    def _update_file_info(self):
        path = self.pcm_path.get().strip()
        if path and os.path.isfile(path):
            size = os.path.getsize(path)
            self.info_var.set(f"大小：{size:,} 字节  ({size / 1048576:.2f} MB)")
        else:
            self.info_var.set("未选择文件")

    # ------------------------------ 开始 / 重新开始 ------------------------------
    def _on_start_button(self):
        if self._closing:
            return
        if not SERIAL_OK:
            messagebox.showerror("缺少依赖",
                                 "未安装 pyserial，请先执行：\n\npip install pyserial")
            return

        # ★ 每次都按当前界面上的选择来
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("提示", "请先选择串口")
            return

        path = self.pcm_path.get().strip()
        if not path or not os.path.isfile(path):
            messagebox.showwarning("提示", "请选择有效的 PCM 文件")
            return

        try:
            baud = int(self.baud_var.get().strip())
            if baud <= 0:
                raise ValueError
        except ValueError:
            messagebox.showerror("错误", "波特率必须是正整数")
            return

        total = os.path.getsize(path)
        if total == 0:
            messagebox.showwarning("提示", "PCM 文件是空的")
            return

        # ---- 先停掉上一次发送（如果有），确保串口已释放 ----
        if self.send_thread is not None and self.send_thread.is_alive():
            if self.current_stop is not None:
                self.current_stop.set()
            if self.current_pause is not None:
                self.current_pause.clear()
            self.send_thread.join(timeout=3.0)
            if self.send_thread.is_alive():
                messagebox.showwarning("提示", "上一次发送尚未停止，请稍后再试")
                return

        # ---- 用当前选择打开串口 ----
        try:
            ser = serial.Serial(
                port=port,
                baudrate=baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,
                write_timeout=2.0,
            )
        except Exception as e:
            self.current_stop = None
            self.current_pause = None
            messagebox.showerror("打开串口失败", f"{port}\n\n{e}")
            self.btn_start.config(text="开始发送")
            self.btn_pause.config(state="disabled", text="暂停")
            return

        # ---- 新会话 ----
        self.session_id += 1
        sid = self.session_id
        stop_ev = threading.Event()
        pause_ev = threading.Event()
        self.current_stop = stop_ev
        self.current_pause = pause_ev

        self.progress["value"] = 0.0
        self.status_var.set(f"发送中... 0 / {total:,} 字节  [{port} @ {baud}]")
        self.btn_start.config(text="重新开始")
        self.btn_pause.config(state="normal", text="暂停")

        self.send_thread = threading.Thread(
            target=self._send_worker,
            args=(ser, path, total, stop_ev, pause_ev, sid),
            daemon=True)
        self.send_thread.start()

    # ------------------------------ 发送线程 ------------------------------
    def _send_worker(self, ser, path, total, stop_ev, pause_ev, sid):
        """一次把整个文件读完并写到串口；写完即结束，不循环。"""
        status, err, sent = "done", "", 0
        try:
            with open(path, "rb") as f:
                while not stop_ev.is_set():
                    if pause_ev.is_set():
                        time.sleep(0.05)
                        continue
                    data = f.read(CHUNK_SIZE)
                    if not data:
                        break
                    ser.write(data)
                    sent += len(data)
                    self.msg_queue.put(("progress", sid, sent, total))
            if stop_ev.is_set() and sent < total:
                status = "stopped"
        except Exception as e:
            status, err = "error", str(e)
        finally:
            try:
                ser.flush()
            except Exception:
                pass
            try:
                ser.close()
            except Exception:
                pass
            self.msg_queue.put(("finished", sid, status, err, sent, total))

    # ------------------------------ 暂停 / 继续 ------------------------------
    def _toggle_pause(self):
        if self.current_pause is None:
            return
        if self.current_pause.is_set():
            self.current_pause.clear()
            self.btn_pause.config(text="暂停")
            self.status_var.set("继续发送...")
        else:
            self.current_pause.set()
            self.btn_pause.config(text="继续")
            self.status_var.set("已暂停")

    # ======================================================================
    # 页面②：ffmpeg 转 PCM
    # ======================================================================
    def _detect_ffmpeg(self):
        exe = shutil.which("ffmpeg")
        if not exe:                                    # 再找脚本同目录
            name = "ffmpeg.exe" if os.name == "nt" else "ffmpeg"
            cand = os.path.join(APP_DIR, name)
            if os.path.isfile(cand):
                exe = cand

        self.ffmpeg_exe = exe
        if exe:
            self.ffmpeg_status_var.set(f"✔ 已检测到 FFmpeg：{exe}")
            self.btn_convert.config(state="normal")
        else:
            self.ffmpeg_status_var.set(
                "✘ 未检测到 FFmpeg，转换功能不可用。\n"
                "  Windows：下载 ffmpeg 后将 bin 目录加入系统 PATH\n"
                "  macOS：brew install ffmpeg     Linux：sudo apt install ffmpeg\n"
                "  也可以把 ffmpeg 可执行文件放到本脚本所在目录。")
            self.btn_convert.config(state="disabled")

    def _choose_audio(self):
        path = filedialog.askopenfilename(
            title="选择音频文件", initialdir=APP_DIR,
            filetypes=[("音频文件",
                        "*.wav *.mp3 *.flac *.aac *.m4a *.ogg *.wma *.opus *.ape"),
                       ("所有文件", "*.*")])
        if path:
            self.audio_path.set(path)
            base = os.path.splitext(os.path.basename(path))[0]
            self.conv_status_var.set(
                f"将输出到：{os.path.join(self.out_dir_var.get(), base + '.pcm')}")

    def _choose_outdir(self):
        d = filedialog.askdirectory(title="选择输出目录",
                                    initialdir=self.out_dir_var.get() or APP_DIR)
        if d:
            self.out_dir_var.set(d)

    # ------------------------------ 日志 ------------------------------
    def _append_log(self, text):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", text)
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def _clear_log(self):
        self.log_text.configure(state="normal")
        self.log_text.delete("1.0", "end")
        self.log_text.configure(state="disabled")

    # ------------------------------ 转换 ------------------------------
    def _start_convert(self):
        if not self.ffmpeg_exe:
            messagebox.showerror("错误", "未检测到 FFmpeg")
            return

        src = self.audio_path.get().strip()
        if not src or not os.path.isfile(src):
            messagebox.showwarning("提示", "请选择有效的音频文件")
            return

        out_dir = self.out_dir_var.get().strip() or APP_DIR
        if not os.path.isdir(out_dir):
            try:
                os.makedirs(out_dir, exist_ok=True)
            except Exception as e:
                messagebox.showerror("错误", f"无法创建输出目录：\n{e}")
                return

        base = os.path.splitext(os.path.basename(src))[0]
        dst = os.path.join(out_dir, base + ".pcm")

        ch_map = {"单声道": "1", "立体声": "2", "保持原样": None}
        channels = ch_map.get(self.channels_var.get(), "1")

        self.btn_convert.config(state="disabled")
        self.conv_status_var.set("正在转换 ...")
        self._append_log(f"\n===== {time.strftime('%H:%M:%S')} 开始转换 =====\n")

        threading.Thread(target=self._convert_worker,
                         args=(src, dst, channels),
                         daemon=True).start()

    def _convert_worker(self, src, dst, channels):
        cmd = [self.ffmpeg_exe, "-hide_banner", "-nostats", "-loglevel", "info",
               "-y", "-i", src, "-vn",
               "-f", "u8", "-acodec", "pcm_u8", "-ar", str(SAMPLE_RATE)]
        if channels:
            cmd += ["-ac", channels]
        cmd.append(dst)

        self.msg_queue.put(("conv_log", "$ " + " ".join(cmd) + "\n"))

        kwargs = {}
        if os.name == "nt":
            kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)

        try:
            proc = subprocess.Popen(
                cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, encoding="utf-8", errors="replace", bufsize=1,
                **kwargs)
            for line in proc.stdout:
                self.msg_queue.put(("conv_log", line))
            proc.wait()

            if proc.returncode != 0:
                self.msg_queue.put(("conv_err",
                                    f"ffmpeg 返回码 {proc.returncode}"))
                return
            if not os.path.isfile(dst) or os.path.getsize(dst) == 0:
                self.msg_queue.put(("conv_err", "转换输出文件为空"))
                return
            self.msg_queue.put(("conv_ok", dst, os.path.getsize(dst)))
        except Exception as e:
            self.msg_queue.put(("conv_err", str(e)))

    # ======================================================================
    # 消息队列轮询 / 界面刷新
    # ======================================================================
    def _poll_queue(self):
        if self._closing:
            return
        try:
            while True:
                self._handle_msg(self.msg_queue.get_nowait())
        except queue.Empty:
            pass
        self.root.after(UI_POLL_MS, self._poll_queue)

    def _handle_msg(self, msg):
        kind = msg[0]

        # ---------- 发送相关（带会话号，丢弃过期消息） ----------
        if kind in ("progress", "finished") and msg[1] != self.session_id:
            return

        if kind == "progress":
            _, _sid, sent, total = msg
            pct = sent * 100.0 / total if total else 0.0
            self.progress["value"] = pct
            if self.current_pause is None or not self.current_pause.is_set():
                self.status_var.set(
                    f"发送中... {sent:,} / {total:,} 字节  ({pct:.1f}%)")

        elif kind == "finished":
            _, _sid, status, err, sent, total = msg
            self.current_stop = None
            self.current_pause = None
            self.btn_pause.config(state="disabled", text="暂停")
            # 按钮文字保持「重新开始」，方便直接再发一遍

            if status == "done":
                self.progress["value"] = 100.0
                self.status_var.set(f"发送完成，共 {total:,} 字节")
            elif status == "stopped":
                self.status_var.set(f"已停止（已发送 {sent:,} / {total:,} 字节）")
            else:
                self.status_var.set("发送出错：" + err)
                messagebox.showerror("发送出错", err)

        # ---------- 转换日志 ----------
        elif kind == "conv_log":
            self._append_log(msg[1])

        # ---------- 转换完成 ----------
        elif kind == "conv_ok":
            _, dst, size = msg
            self.btn_convert.config(state="normal")
            self.conv_status_var.set("转换完成，已填入发送页")
            self._append_log(f"\n✔ 转换完成：{dst}\n   文件大小：{size:,} 字节\n")
            self.pcm_path.set(dst)          # 自动填到第一个界面
            self._update_file_info()

        # ---------- 转换失败 ----------
        elif kind == "conv_err":
            _, err = msg
            self.btn_convert.config(state="normal")
            self.conv_status_var.set("转换失败")
            self._append_log(f"\n✘ 转换失败：{err}\n")
            messagebox.showerror("转换失败", err)

    # ======================================================================
    # 退出
    # ======================================================================
    def _on_close(self):
        self._closing = True
        if self.current_stop is not None:
            self.current_stop.set()
        if self.current_pause is not None:
            self.current_pause.clear()

        t = self.send_thread
        if t is not None and t.is_alive():
            t.join(timeout=2.0)

        self.root.destroy()


def main():
    root = tk.Tk()
    try:
        style = ttk.Style()
        for theme in ("vista", "clam", "default"):
            if theme in style.theme_names():
                style.theme_use(theme)
                break
    except Exception:
        pass

    PcmTool(root)
    root.mainloop()


if __name__ == "__main__":
    main()