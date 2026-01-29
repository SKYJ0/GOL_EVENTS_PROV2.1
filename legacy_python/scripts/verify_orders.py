import os
import re
import csv
import threading
import datetime
import customtkinter as ctk
import requests
import subprocess
import sys
from tkinter import filedialog, messagebox
from collections import defaultdict

# نحاول استيراد openpyxl للتعامل مع ملفات Excel
try:
    import openpyxl
except ImportError:
    openpyxl = None

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "2.9"
FIREBASE_URL = "https://golevents1-default-rtdb.europe-west1.firebasedatabase.app"
ACCENT_COLOR = "#1f6aa5" 
BG_COLOR = "#0b0b0b"
CARD_BG = "#161616"
CARD_BORDER = "#252525"
SUCCESS_COLOR = "#00FF94"
ERROR_COLOR = "#FF4B4B"
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
        StrictIDVerifierApp(self)

# --- GUI INTERFACE ---
class StrictIDVerifierApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - STRICT VERIFIER PRO 🔍")
        self.master.geometry("950x850")
        self.master.configure(fg_color=BG_COLOR)
        
        self.ALLOWED_COLUMNS = ["id", "transactionid", "order id", "orderid"]
        self.sales_files = []
        
        # Header
        header_frame = ctk.CTkFrame(self.master, fg_color="transparent")
        header_frame.pack(fill="x", padx=40, pady=(30, 20))
        ctk.CTkLabel(header_frame, text="🔍 STRICT ORDER VERIFIER", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header_frame, text="Targeted scan for missing or duplicated Order IDs.", font=("Arial", 13), text_color="gray60").pack(anchor="w")

        # Action Frame
        action_frame = ctk.CTkFrame(self.master, fg_color=CARD_BG, corner_radius=15, border_width=1, border_color=CARD_BORDER)
        action_frame.pack(fill="x", padx=40, pady=10)

        self.path_var = ctk.StringVar(value="Select match folder to scan...")
        ctk.CTkEntry(action_frame, textvariable=self.path_var, font=("Arial", 12), width=600, height=45, fg_color="#0d0d0d", state="readonly").grid(row=0, column=0, padx=20, pady=20)
        ctk.CTkButton(action_frame, text="BROWSE FOLDER", width=140, height=45, fg_color=ACCENT_COLOR, font=("Arial", 12, "bold"), command=self.browse_folder).grid(row=0, column=1, padx=(0, 20))

        # Files Selection
        self.btn_sales = ctk.CTkButton(self.master, text="📂 SELECT SALES FILES (CSV / XLSX)", height=50, 
                                        fg_color="#2A2F3E", hover_color="#3a3f4e", font=("Arial", 13, "bold"),
                                        command=self.get_sales_files)
        self.btn_sales.pack(pady=10, padx=40, fill="x")
        self.sales_lbl = ctk.CTkLabel(self.master, text="No sales files selected", text_color="gray45", font=("Arial", 11))
        self.sales_lbl.pack()

        # Run Button
        self.run_btn = ctk.CTkButton(self.master, text="🚀 START TARGETED SCAN", height=60, 
                                     fg_color=SUCCESS_COLOR, hover_color="#059669", text_color="black",
                                     font=("Arial", 16, "bold"), command=self.start_thread)
        self.run_btn.pack(pady=20, padx=40, fill="x")

        # Log Area
        self.log_area = ctk.CTkTextbox(self.master, font=("Consolas", 12), fg_color="#050505", 
                                        text_color=SUCCESS_COLOR, border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(fill="both", expand=True, padx=40, pady=(0, 30))

        # Footer
        ctk.CTkLabel(self.master, text="Created by Omar", font=("Arial", 11, "italic"), text_color="gray30").place(relx=0.5, rely=0.98, anchor="s")

    def clean_header(self, h):
        if not h: return ""
        return re.sub(r'^[^a-zA-Z0-9]+', '', str(h)).strip().lower()

    def log(self, msg):
        self.log_area.configure(state="normal")
        self.log_area.insert("end", f"> {msg}\n")
        self.log_area.see("end")
        self.log_area.configure(state="disabled")

    def browse_folder(self):
        p = filedialog.askdirectory()
        if p: self.path_var.set(p)

    def get_sales_files(self):
        f = filedialog.askopenfilenames(filetypes=[("Sales Data", "*.csv *.xlsx")])
        if f: 
            self.sales_files = list(f)
            self.sales_lbl.configure(text=f"{len(f)} files selected", text_color=SUCCESS_COLOR)

    def extract_from_excel(self, file_path):
        extracted = set()
        if openpyxl is None:
            self.log("❌ Error: 'openpyxl' library not found.")
            return extracted
        try:
            wb = openpyxl.load_workbook(file_path, data_only=True)
            ws = wb.active
            rows = list(ws.rows)
            if not rows: return extracted
            header = [self.clean_header(cell.value) for cell in rows[0]]
            id_indices = [i for i, val in enumerate(header) if val in self.ALLOWED_COLUMNS]
            for row in rows[1:]:
                for idx in id_indices:
                    val = str(row[idx].value).strip() if row[idx].value else ""
                    if len(val) >= 5: extracted.add(val)
        except Exception as e: self.log(f"❌ Excel Error: {e}")
        return extracted

    def extract_from_csv(self, file_path):
        extracted = set()
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                header_line = f.readline()
                f.seek(0)
                delim = '\t' if '\t' in header_line else ','
                reader = csv.DictReader(f, delimiter=delim)
                id_cols = [c for c in reader.fieldnames if self.clean_header(c) in self.ALLOWED_COLUMNS]
                for row in reader:
                    for col in id_cols:
                        val = str(row[col]).strip()
                        if len(val) >= 5: extracted.add(val)
        except Exception as e: self.log(f"❌ CSV Error: {e}")
        return extracted

    def start_thread(self):
        if "Select" in self.path_var.get() or not self.sales_files:
            messagebox.showwarning("Missing Info", "Please select scan folder and sales files.")
            return
        self.run_btn.configure(state="disabled", text="SCANNING...")
        self.log_area.configure(state="normal"); self.log_area.delete("1.0", "end"); self.log_area.configure(state="disabled")
        threading.Thread(target=self.run_verify, daemon=True).start()

    def run_verify(self):
        match_path = self.path_var.get()
        all_sales_ids = set()
        self.log("Extracting IDs from sales files...")
        
        for f in self.sales_files:
            ids = self.extract_from_excel(f) if f.lower().endswith('.xlsx') else self.extract_from_csv(f)
            all_sales_ids.update(ids)
            self.log(f"📄 {os.path.basename(f)}: Found {len(ids)} unique IDs")

        self.log("Scanning local directories...")
        id_matches = defaultdict(list)
        try:
            all_local_folders = []
            for root, dirs, files in os.walk(match_path):
                for d in dirs: all_local_folders.append(d)

            for s_id in all_sales_ids:
                pattern = re.compile(r'\b' + re.escape(s_id) + r'\b', re.IGNORECASE)
                for folder_name in all_local_folders:
                    if pattern.search(folder_name):
                        id_matches[s_id].append(folder_name)
        except Exception as e:
            self.log(f"❌ Scan Error: {e}"); return

        missing = [sid for sid in all_sales_ids if len(id_matches.get(sid, [])) == 0]
        dupes = {sid: folders for sid, folders in id_matches.items() if len(folders) > 1}

        self.log("\n" + "="*40)
        self.log(f"✅ PERFECT MATCHES : {len(id_matches) - len(dupes)}")
        self.log(f"❌ MISSING IDs     : {len(missing)}")
        self.log(f"🚨 DUPLICATED IDs  : {len(dupes)}")
        self.log("="*40)

        # Reports to Desktop
        desktop = os.path.join(os.path.expanduser("~"), "Desktop")
        ts = datetime.datetime.now().strftime('%H%M%S')
        
        if missing:
            with open(os.path.join(desktop, f"Missing_Orders_{ts}.txt"), "w") as f: f.write("\n".join(sorted(missing)))
            self.log("💾 Missing IDs saved to Desktop.")
        
        if dupes:
            with open(os.path.join(desktop, f"Duplicated_Orders_{ts}.txt"), "w") as f:
                f.write("ORDER ID | FOLDER MATCHES\n" + "-"*50 + "\n")
                for sid, folders in dupes.items(): f.write(f"{sid} -> {', '.join(folders)}\n")
            self.log("🚨 Duplicates detected! Report saved to Desktop.")

        self.run_btn.configure(state="normal", text="🚀 START TARGETED SCAN")
        if not missing and not dupes: messagebox.showinfo("Success", "Perfect match! No missing or duplicated IDs.")

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1])
        app.mainloop()
    else:
        ctk.set_appearance_mode("Dark")
        root = ctk.CTk()
        root.withdraw()
        messagebox.showerror("Unauthorized Access", "Please launch via GOLEVENTS Hub.")
        sys.exit()