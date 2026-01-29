import os
import re
import shutil
import threading
import customtkinter as ctk
import requests
import subprocess
import sys
from PyPDF2 import PdfReader, PdfWriter
import pdfplumber
from tkinter import messagebox, filedialog
from concurrent.futures import ThreadPoolExecutor, as_completed

# ================== [ SECURITY & DESIGN CONFIG ] ==================
VERSION = "3.2.1"
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
        self.status_lbl = ctk.CTkLabel(self.splash, text="Initializing Split Engine...", font=("Arial", 13), text_color="gray")
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
        PDFSplitterProApp(self)

# --- CORE LOGIC ENGINE ---
def extract_ticket_info(pdf_path, page_num):
    try:
        with pdfplumber.open(pdf_path) as pdf:
            if page_num >= len(pdf.pages): return None
            page = pdf.pages[page_num]
            text = page.extract_text()
            if not text: return None
            
            # Pattern 1: Table Style (Settore/Fila/Posto)
            p1 = r'SETTORE/BLOCK\s+FILA/ROW\s+POSTO/SEAT\s*\n(?:.*?\n)?\s*(\d+)\s+(\d+)\s+(\d+)'
            m1 = re.search(p1, text, re.MULTILINE)
            if m1: return (m1.group(1), m1.group(2), m1.group(3))
            
            # Pattern 2: Inline Style (SETT. 2BC FILA 18)
            p2 = r'SETT\.\s+([A-Z0-9]+)\s+FILA\s+(\d+)\s+POSTO\s+(\d+)-([A-Z])'
            m2 = re.search(p2, text)
            if m2: return (m2.group(1), m2.group(2), f"{m2.group(3)}{m2.group(4)}")
            
            return None
    except: return None

def is_acmilan_ticket(filename):
    pattern = r'^[A-Z]+-[A-Z]+_\d+_(\d+)_(\d+)_(\d+)_[A-Z]+_\d+\.pdf$'
    return re.match(pattern, filename, re.IGNORECASE) is not None

# --- GUI INTERFACE ---
class PDFSplitterProApp:
    def __init__(self, master):
        self.master = master
        self.master.title("GOLEVENTS - MULTI-CORE SPLITTER PRO ✂️")
        self.master.geometry("1000x850")
        self.master.configure(fg_color=BG_COLOR)

        # Header
        header = ctk.CTkFrame(self.master, fg_color="transparent")
        header.pack(fill="x", padx=40, pady=(30, 20))
        ctk.CTkLabel(header, text="✂️ POWER TICKET SPLITTER", font=("Arial", 30, "bold"), text_color=ACCENT_COLOR).pack(anchor="w")
        ctk.CTkLabel(header, text="High-speed extraction for multi-page PDF tickets.", font=("Arial", 13), text_color="gray60").pack(anchor="w")

        # Input Frame
        path_box = ctk.CTkFrame(self.master, fg_color=CARD_BG, corner_radius=15, border_width=1, border_color=CARD_BORDER)
        path_box.pack(fill="x", padx=40, pady=10)

        self.path_var = ctk.StringVar(value="Select folder with tickets to split...")
        ctk.CTkEntry(path_box, textvariable=self.path_var, width=680, height=45, state="readonly", fg_color="#0d0d0d").grid(row=0, column=0, padx=20, pady=20)
        ctk.CTkButton(path_box, text="BROWSE FOLDER", width=140, height=45, command=self.browse, fg_color=ACCENT_COLOR).grid(row=0, column=1, padx=(0, 20))

        # Run Button
        self.run_btn = ctk.CTkButton(self.master, text="🚀 START BULK SPLITTING", height=60, 
                                      fg_color="#10b981", hover_color="#059669", text_color="black",
                                      font=("Arial", 16, "bold"), command=self.start_process)
        self.run_btn.pack(pady=20, padx=40, fill="x")

        # Logs
        self.log_area = ctk.CTkTextbox(self.master, font=("Consolas", 12), fg_color="#050505", text_color="#00FF94", border_width=1, border_color=CARD_BORDER)
        self.log_area.pack(fill="both", expand=True, padx=40, pady=(0, 30))

    def browse(self):
        p = filedialog.askdirectory()
        if p: self.path_var.set(p)

    def log(self, msg):
        self.log_area.configure(state="normal"); self.log_area.insert("end", f"> {msg}\n"); self.log_area.see("end"); self.log_area.configure(state="disabled")
        self.master.update()

    def start_process(self):
        folder = self.path_var.get()
        if "Select" in folder:
            messagebox.showwarning("Warning", "Please select a folder first.")
            return
        self.run_btn.configure(state="disabled", text="PROCESSING...")
        self.log_area.configure(state="normal"); self.log_area.delete("1.0", "end"); self.log_area.configure(state="disabled")
        threading.Thread(target=self.worker, args=(folder,), daemon=True).start()

    def process_page_worker(self, pdf_path, page_num, output_folder, reader):
        try:
            info = extract_ticket_info(pdf_path, page_num)
            fname = f"{info[0]}-{info[1]}-{info[2]}.pdf" if info else f"manual_check_page_{page_num+1}.pdf"
            
            writer = PdfWriter()
            writer.add_page(reader.pages[page_num])
            out_p = os.path.join(output_folder, fname)
            with open(out_p, 'wb') as f: writer.write(f)
            return f"   [✓] {fname}"
        except Exception as e:
            return f"   [✗] Error on page {page_num+1}: {e}"

    def worker(self, folder):
        import time
        start_t = time.time()
        output_folder = os.path.join(folder, "Split_Result")
        os.makedirs(output_folder, exist_ok=True)

        pdf_files = [f for f in os.listdir(folder) if f.lower().endswith('.pdf')]
        self.log(f"Detected {len(pdf_files)} PDF files. Initializing Threads...")

        total_pages = 0
        for pdf_name in pdf_files:
            pdf_path = os.path.join(folder, pdf_name)
            self.log(f"Scanning: {pdf_name}")
            
            # Special Handling for AC MILAN (Fast Copy)
            if is_acmilan_ticket(pdf_name):
                match = re.match(r'^[A-Z]+-[A-Z]+_\d+_(\d+)_(\d+)_(\d+)_[A-Z]+_\d+\.pdf$', pdf_name, re.IGNORECASE)
                if match:
                    new_name = f"{match.group(1)}-{match.group(2)}-{match.group(3)}.pdf"
                    shutil.copy2(pdf_path, os.path.join(output_folder, new_name))
                    self.log(f"   [🚀] Smart-Copy: {new_name}")
                    continue

            # Standard Splitting with Multi-threading
            try:
                reader = PdfReader(pdf_path)
                pages = len(reader.pages)
                with ThreadPoolExecutor(max_workers=4) as executor:
                    futures = [executor.submit(self.process_page_worker, pdf_path, i, output_folder, reader) for i in range(pages)]
                    for future in as_completed(futures):
                        self.log(future.result())
                        total_pages += 1
            except Exception as e:
                self.log(f"   [!] Critical Error on {pdf_name}: {e}")

        end_t = time.time()
        self.log(f"\n✅ FINISHED! Processed in {end_t - start_t:.2f} seconds.")
        self.log("Process complete. All files are ready.")
        self.run_btn.configure(state="normal", text="🚀 START BULK SPLITTING", fg_color="#10b981")
        messagebox.showinfo("Done", f"Success! Tickets saved in:\n{output_folder}")
        try: os.startfile(output_folder)
        except: pass

if __name__ == "__main__":
    if len(sys.argv) > 1:
        app = SecurityCheck(sys.argv[1]); app.mainloop()
    else:
        ctk.set_appearance_mode("Dark"); root = ctk.CTk(); root.withdraw()
        messagebox.showerror("Error", "Launch via GOLEVENTS Hub."); sys.exit()