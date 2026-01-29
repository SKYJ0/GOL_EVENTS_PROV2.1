import os
import re
import threading
import customtkinter as ctk
import requests
import subprocess
import sys
import qrcode
from PIL import Image
from tkinter import messagebox, filedialog

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "3.2.0"
FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
ACCENT_COLOR = "#1f6aa5" 
BG_COLOR = "#0b0b0b"
CARD_BG = "#161616"
CARD_BORDER = "#252525"
# =================================================================

def get_hwid():
    try:
        cmd = 'wmic baseboard get serialnumber'
        hwid = str(subprocess.check_output(cmd, shell=True), 'utf-8').split('\n')[1].strip()
        return hwid
    except:
        import hashlib
        return hashlib.sha256(os.environ['COMPUTERNAME'].encode()).hexdigest()[:16]

# --- SECURITY WRAPPER ---
class SecurityCheck(ctk.CTk):
    def __init__(self, key):
        super().__init__()
        self.withdraw()
        self.license_key = key
        
        self.splash = ctk.CTkToplevel(self)
        self.splash.title("Authenticating...")
        self.splash.geometry("450x250")
        self.splash.configure(fg_color=BG_COLOR)
        self.splash.overrideredirect(True)
        
        x = (self.winfo_screenwidth() // 2) - 225
        y = (self.winfo_screenheight() // 2) - 125
        self.splash.geometry(f"+{x}+{y}")

        ctk.CTkLabel(self.splash, text="GOLEVENTS SECURE 👑", font=("Arial", 24, "bold"), text_color=ACCENT_COLOR).pack(pady=(45, 5))
        self.status_lbl = ctk.CTkLabel(self.splash, text="Verifying Module Access...", font=("Arial", 13), text_color="gray")
        self.status_lbl.pack()
        
        self.progress = ctk.CTkProgressBar(self.splash, width=350, height=12, progress_color=ACCENT_COLOR); self.progress.set(0); self.progress.pack(pady=30)
        self.after(500, self.verify)

    def verify(self):
        hwid = get_hwid()
        try:
            res = requests.get(f"{FIREBASE_URL}/keys/{self.license_key}.json").json()
            if res and res.get("status") == "active" and res.get("hwid") == hwid:
                self.progress.set(1.0); self.status_lbl.configure(text="ACCESS GRANTED", text_color="#2ecc71")
                self.after(800, self.launch_main_tool)
            else:
                messagebox.showerror("Security Alert", "Unauthorized Access!"); sys.exit()
        except: sys.exit()

    def launch_main_tool(self):
        self.splash.destroy(); self.deiconify()
        QRCodeGeneratorApp(self)

# --- CORE LOGIC ---
def sanitize_filename(filename):
    filename = re.sub(r'[<>:"/\\|?*]', '_', filename)
    filename = filename.strip('. ')
    if len(filename) > 100: filename = filename[:100]
    return filename if filename else "qr_code"

# --- GUI INTERFACE ---
class QRCodeGeneratorApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - QR GENERATOR PRO 🔳")
        self.master.geometry("850x850")
        self.master.configure(fg_color=BG_COLOR)

        # Header
        header = ctk.CTkFrame(self.master, fg_color="transparent")
        header.pack(fill="x", padx=40, pady=(30, 10))
        ctk.CTkLabel(header, text="🔳 BATCH QR GENERATOR", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header, text="Generate 1035x1035 high-res QR codes from a list.", font=("Arial", 13), text_color="gray60").pack(anchor="w")

        # Input Area
        ctk.CTkLabel(self.master, text="Paste your codes list (one per line):", font=("Arial", 12, "bold"), text_color="gray75").pack(anchor="w", padx=40)
        self.input_area = ctk.CTkTextbox(self.master, font=("Consolas", 13), fg_color=CARD_BG, 
                                          border_color=CARD_BORDER, border_width=1, corner_radius=15)
        self.input_area.pack(pady=10, padx=40, fill="both", expand=True)

        # Progress
        self.prog_bar = ctk.CTkProgressBar(self.master, width=770, height=10, progress_color="#10b981")
        self.prog_bar.set(0)
        self.prog_bar.pack(pady=10)

        # Buttons
        btn_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        btn_frame.pack(fill="x", padx=40, pady=10)

        self.run_btn = ctk.CTkButton(btn_frame, text="🚀 CHOOSE FOLDER & GENERATE", height=55, 
                                      fg_color="#10b981", hover_color="#059669", text_color="black",
                                      font=("Arial", 16, "bold"), command=self.start_process)
        self.run_btn.pack(side="left", fill="x", expand=True, padx=(0, 10))

        self.clear_btn = ctk.CTkButton(btn_frame, text="🗑️ CLEAR", height=55, width=120,
                                       fg_color="#e74c3c", hover_color="#c0392b", 
                                       command=lambda: self.input_area.delete("1.0", "end"))
        self.clear_btn.pack(side="left")

        # Logs
        self.log_area = ctk.CTkTextbox(self.master, height=120, fg_color="#050505", text_color="#00FF94", 
                                        font=("Consolas", 12), state="disabled", border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(pady=(10, 30), padx=40, fill="x")

    def log(self, msg):
        self.log_area.configure(state="normal"); self.log_area.insert("end", f"> {msg}\n"); self.log_area.see("end"); self.log_area.configure(state="disabled")
        self.master.update()

    def start_process(self):
        raw_text = self.input_area.get("1.0", "end").strip()
        if not raw_text:
            messagebox.showwarning("Empty", "Please paste some codes first!")
            return

        out_dir = filedialog.askdirectory(title="Select folder to save QR codes")
        if not out_dir: return

        codes = [line.strip() for line in raw_text.split("\n") if line.strip()]
        self.run_btn.configure(state="disabled", text="GENERATING...")
        threading.Thread(target=self.worker, args=(codes, out_dir), daemon=True).start()

    def worker(self, codes, out_dir):
        total = len(codes)
        self.log(f"Starting batch generation for {total} items...")
        
        for i, data in enumerate(codes):
            try:
                # Logic QR
                qr = qrcode.QRCode(
                    version=None,
                    error_correction=qrcode.constants.ERROR_CORRECT_H,
                    box_size=20,
                    border=0,
                )
                qr.add_data(data)
                qr.make(fit=True)

                img = qr.make_image(fill_color="black", back_color="white")
                img_res = img.resize((1035, 1035), Image.Resampling.LANCZOS)

                fname = sanitize_filename(data) + ".png"
                img_res.save(os.path.join(out_dir, fname))
                
                self.log(f"Saved: {fname}")
                self.prog_bar.set((i + 1) / total)
            except Exception as e:
                self.log(f"FAILED: {data} | {e}")

        self.log("\n✅ ALL DONE!")
        self.run_btn.configure(state="normal", text="🚀 CHOOSE FOLDER & GENERATE")
        messagebox.showinfo("Done", f"Generated {total} QR codes successfully!")
        try: os.startfile(out_dir)
        except: pass

if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1]); app.mainloop()
    else:
        ctk.set_appearance_mode("Dark"); root = ctk.CTk(); root.withdraw()
        messagebox.showerror("Error", "Launch via GOLEVENTS Hub."); sys.exit()