import os
import re
import customtkinter as ctk
import requests
import subprocess
import sys
from tkinter import messagebox

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "2.9"
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
        self.splash.title("Authenticating Module...")
        self.splash.geometry("450x250")
        self.splash.configure(fg_color=BG_COLOR)
        self.splash.overrideredirect(True)
        
        x = (self.winfo_screenwidth() // 2) - 225
        y = (self.winfo_screenheight() // 2) - 125
        self.splash.geometry(f"+{x}+{y}")

        ctk.CTkLabel(self.splash, text="GOLEVENTS SECURE 👑", font=("Arial", 24, "bold"), text_color=ACCENT_COLOR).pack(pady=(45, 5))
        self.status_lbl = ctk.CTkLabel(self.splash, text="Verifying Module Access...", font=("Arial", 13), text_color="gray")
        self.status_lbl.pack()
        
        self.progress = ctk.CTkProgressBar(self.splash, width=350, height=12, progress_color=ACCENT_COLOR)
        self.progress.set(0)
        self.progress.pack(pady=30)
        
        self.after(500, self.verify)

    def verify(self):
        hwid = get_hwid()
        try:
            res = requests.get(f"{FIREBASE_URL}/keys/{self.license_key}.json").json()
            if res and res.get("status") == "active" and res.get("hwid") == hwid:
                self.progress.set(1.0)
                self.status_lbl.configure(text="ACCESS GRANTED", text_color="#2ecc71")
                self.after(800, self.launch_main_tool)
            else:
                messagebox.showerror("Security Alert", "Unauthorized Access!\nPlease launch through GOLEVENTS Hub.")
                sys.exit()
        except:
            sys.exit()

    def launch_main_tool(self):
        self.splash.destroy()
        self.deiconify()
        SeatExpanderApp(self)

# --- ORIGINAL LOGIC ENGINE ---
def normalize(line):
    return re.sub(r"\s+", " ", line.strip())

def expand_line(line):
    line = normalize(line)
    results = []
    # Case 1: From XS to YS
    m = re.search(r"(.*?)(?:From|from)?\s*(\d+)([SDsd])\s+to\s+(\d+)\3", line)
    if m:
        prefix = m.group(1).strip()
        start, end = int(m.group(2)), int(m.group(4))
        suffix = m.group(3).upper()
        if prefix and not prefix.endswith("-"): prefix += "-"
        step = 1 if start <= end else -1
        for i in range(start, end + step, step): results.append(f"{prefix}{i}{suffix}")
        return results
    # Case 2: XS / YS
    m = re.search(r"(.*?)(\d+)([SDsd])\s*/\s*(\d+)\3", line)
    if m:
        prefix = m.group(1).strip()
        start, end = int(m.group(2)), int(m.group(4))
        suffix = m.group(3).upper()
        if prefix and not prefix.endswith("-"): prefix += "-"
        step = 1 if start <= end else -1
        for i in range(start, end + step, step): results.append(f"{prefix}{i}{suffix}")
        return results
    # Case 3: Single value
    if re.fullmatch(r".*\d+[SDsd]", line):
        val = line.upper().strip()
        return [val]
    return []

# --- UPDATED GUI INTERFACE ---
class SeatExpanderApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - SEAT EXPANDER PRO 🪑")
        self.master.geometry("750x800")
        self.master.configure(fg_color=BG_COLOR)

        # Header
        header_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        header_frame.pack(fill="x", padx=40, pady=(30, 20))
        
        ctk.CTkLabel(header_frame, text="💺 SEAT EXPANDER PRO", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header_frame, text="Patterns: 'From 1S to 5S' or 'Row1 10D/15D'", font=("Arial", 13), text_color="gray60").pack(anchor="w")
        
        # Input Area
        self.input_area = ctk.CTkTextbox(self.master, font=("Consolas", 13), fg_color=CARD_BG, 
                                          border_color=CARD_BORDER, border_width=1, corner_radius=15)
        self.input_area.pack(pady=10, padx=40, fill="both", expand=True)

        # Buttons Frame
        btn_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        btn_frame.pack(fill="x", padx=40, pady=20)

        self.gen_btn = ctk.CTkButton(btn_frame, text="🚀 EXPAND & SAVE TO DESKTOP", height=55, 
                                     fg_color=ACCENT_COLOR, font=("Arial", 15, "bold"),
                                     command=self.process_expansion)
        self.gen_btn.pack(side="left", fill="x", expand=True, padx=(0, 10))

        self.clear_btn = ctk.CTkButton(btn_frame, text="🗑️ CLEAR", height=55, width=120,
                                       fg_color="#e74c3c", hover_color="#c0392b", 
                                       command=lambda: self.input_area.delete("1.0", "end"))
        self.clear_btn.pack(side="left")

        # Log Area
        self.log_area = ctk.CTkTextbox(self.master, height=120, fg_color="#050505", text_color="#00FF94", 
                                        font=("Consolas", 12), state="disabled", border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(pady=(0, 30), padx=40, fill="x")

    def log(self, msg):
        self.log_area.configure(state="normal")
        self.log_area.insert("end", f"> {msg}\n")
        self.log_area.see("end")
        self.log_area.configure(state="disabled")

    def process_expansion(self):
        raw_text = self.input_area.get("1.0", "end").strip()
        if not raw_text:
            messagebox.showwarning("Empty", "Please paste some patterns first!")
            return

        lines = raw_text.split("\n")
        all_results = []
        for line in lines:
            if line.strip():
                expanded = expand_line(line)
                all_results.extend(expanded)

        if all_results:
            desktop = os.path.join(os.path.expanduser("~"), "Desktop")
            output_path = os.path.join(desktop, "Expanded_Seats.txt")
            with open(output_path, "w", encoding="utf-8") as f:
                f.write("\n".join(all_results))
            
            self.log(f"Generated {len(all_results)} seats.")
            self.log(f"Saved: Desktop/Expanded_Seats.txt")
            messagebox.showinfo("Success", f"Generated {len(all_results)} seats!\nSaved to Desktop.")
        else:
            self.log("No valid patterns found. Check your format.")

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk()
        root.withdraw()
        messagebox.showerror("Unauthorized", "Please launch via GOLEVENTS Hub.")
        sys.exit()